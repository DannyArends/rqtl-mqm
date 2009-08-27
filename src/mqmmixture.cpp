/**********************************************************************
 *
 * mqmmixture.cpp
 *
 * copyright (c) 2009 Ritsert Jansen, Danny Arends, Pjotr Prins and Karl W Broman
 *
 * last modified Apr, 2009
 * first written Feb, 2009
 *
 * Original version R.C Jansen
 * first written <2000 (unknown)
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License,
 *     version 3, as published by the Free Software Foundation.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but without any warranty; without even the implied warranty of
 *     merchantability or fitness for a particular purpose.  See the GNU
 *     General Public License, version 3, for more details.
 *
 *     A copy of the GNU General Public License, version 3, is available
 *     at http://www.r-project.org/Licenses/GPL-3
 *
 * Mixture functions
 *
 **********************************************************************/

#include "mqm.h"

/*
 * ML estimation of recombination frequencies via EM; calculation of multilocus
 * genotype probabilities; ignorance of unlikely genotypes. Called by the
 * mqmscan.  maximum-likelihood estimation of recombination frequencies via the
 * EM algorithm, using multilocus information (default: the recombination
 * frequencies are not estimated but taken from mqm.in)
 *
 * When reestimate is 'n' the method is skipped
 */
double rmixture(cmatrix marker, vector weight, vector r,
                cvector position, ivector ind, int Nind, int Naug, int Nmark,
                vector *mapdistance, char reestimate, MQMCrossType crosstype,
                int verbose) {
  int i, j;
  int iem= 0;
  double Nrecom, oldr=0.0, newr, rdelta=1.0;
  double maximum = 0.0;
  float last_step = 0.0;
  vector indweight;
  indweight = newvector(Nind);
  vector distance;
  distance = newvector(Nmark+1);

  if (reestimate=='n') {
    if (verbose==1) {
      Rprintf("INFO: recombination parameters are not re-estimated\n");
    }
    for (j=0; j<Nmark; j++) {
      if (maximum < (*mapdistance)[j]) {
        maximum = (*mapdistance)[j];
      }
    }
  } else {
    if (verbose==1) {
      Rprintf("INFO: recombination parameters are re-estimated\n");
    }
    //Reestimation of map now works
    while ((iem<1000)&&(rdelta>0.0001)) {
      iem+=1;
      rdelta= 0.0;
      /* calculate weights = conditional genotype probabilities */
      for (i=0; i<Naug; i++) weight[i]=1.0;
      for (j=0; j<Nmark; j++) {
        if ((position[j]==MLEFT)||(position[j]==MUNLINKED))
          for (i=0; i<Naug; i++)
            if (marker[j][i]==MH) weight[i]*= 0.5;
            else weight[i]*= 0.25;
        if ((position[j]==MLEFT)||(position[j]==MMIDDLE))
          for (i=0; i<Naug; i++) {
            // [pjotr:] same problem described below, why define if not used? FIXME
            // double calc_i = prob(marker, r, i, j, marker[j+1][i], crosstype, 0, 0, 0);
            double calc_i = prob(marker, r, i, j, marker[j+1][i], crosstype, 0);
            weight[i]*=calc_i;
          }
      }
      for (i=0; i<Nind; i++) {
        indweight[i]= 0.0;
      }
      for (i=0; i<Naug; i++) {
        indweight[ind[i]]+=weight[i];
      }
      for (i=0; i<Naug; i++) {
        weight[i]/=indweight[ind[i]];
      }
      for (j=0; j<Nmark; j++) {
        if ((position[j]==MLEFT)||(position[j]==MMIDDLE)) {
          newr= 0.0;
          for (i=0; i<Naug; i++) {
            Nrecom= fabs((double)marker[j][i]-marker[j+1][i]);
            if ((marker[j][i]==MH)&&(marker[j+1][i]==MH))
              Nrecom= 2.0*r[j]*r[j]/(r[j]*r[j]+(1-r[j])*(1-r[j]));
            newr+= Nrecom*weight[i];
          }
          if (reestimate=='y' && position[j]!=MRIGHT) { //only update if it isn't the last marker of a chromosome ;)
            oldr=r[j];
            r[j]= newr/(2.0*Nind);
            rdelta+=pow(r[j]-oldr, 2.0);
          } else rdelta+=0.0;
        }
      }
    }

    /*   print new estimates of recombination frequencies */

    //Rprintf("INFO: Reestimate? %c\n", reestimate);
    //Rprintf("INFO: looping over all markers %d\n", Nmark);
    for (j=0; j<Nmark; j++) {
      if (position[j+1]==MRIGHT) {
        last_step = (*mapdistance)[j+1]-(*mapdistance)[j];
      }
      if (position[j]!=MLEFT) {
        if (position[j]!=MRIGHT) {
          (*mapdistance)[j]= -50*log(1-2.0*r[j])+(*mapdistance)[j-1];
        } else {
          (*mapdistance)[j]= (*mapdistance)[j-1]+last_step;
        }
      } else {
        (*mapdistance)[j]= -50*log(1-2.0*r[j]);
      }
      if (maximum < (*mapdistance)[j]) {
        maximum = (*mapdistance)[j];
      }
      //Rprintf("r(%d)= %f -> %f\n", j, r[j], (*mapdistance)[j]);
    }
  }
  if (verbose==1) {
    Rprintf("INFO: Re-estimation of the genetic map took %d iterations, to reach a rdelta of %f\n", iem, rdelta);
  }
  Free(indweight);
  freevector(distance);
  return maximum;
}


/* ML estimation of parameters in mixture model via EM; maximum-likelihood
 * estimation of parameters in the mixture model via the EM algorithm, using
 * multilocus information, but assuming known recombination frequencies
*/
double QTLmixture(cmatrix loci, cvector cofactor, vector r, cvector position,
                  vector y, ivector ind, int Nind, int Naug,
                  int Nloci,
                  double *variance, int em, vector *weight, const bool useREML, char fitQTL, char dominance, MQMCrossType crosstype, int verbose) {
  //if(verbose==1){Rprintf("QTLmixture called\n");}
  int iem= 0, newNaug, i, j;
  char varknown, biasadj='n';
  double oldlogL=-10000, delta=1.0, calc_i, logP=0.0, Pscale=1.75;
  vector indweight, Ploci, Fy;

  indweight= newvector(Nind);
  newNaug= (fitQTL=='n' ? Naug : 3*Naug);
  Fy= newvector(newNaug);
  logP= Nloci*log(Pscale); // only for computational accuracy
  varknown= (((*variance)==-1.0) ? 'n' : 'y' );
  Ploci= newvector(newNaug);
#ifndef STANDALONE
  R_CheckUserInterrupt(); /* check for ^C */
  //R_ProcessEvents();
  R_FlushConsole();
#endif
  if ((useREML)&&(varknown=='n')) {
//		Rprintf("INFO: REML\n");
  }
  if (!useREML) {
//		Rprintf("INFO: ML\n");
    varknown='n';
    biasadj='n';
  }
  for (i=0; i<newNaug; i++) {
    Ploci[i]= 1.0;
  }
  if (fitQTL=='n') {
    //Rprintf("FitQTL=N\n");
    for (j=0; j<Nloci; j++) {
      for (i=0; i<Naug; i++)
        Ploci[i]*= Pscale;
      //Here we have ProbLeft
      if ((position[j]==MLEFT)||(position[j]==MUNLINKED)) {
        for (i=0; i<Naug; i++) {
          // calc_i= prob(loci, r, i, j, MH, crosstype, 0, 1);
          calc_i = start_prob(crosstype, loci[j][i]);
          Ploci[i]*= calc_i;
        }
      }
      if ((position[j]==MLEFT)||(position[j]==MMIDDLE)) {
        for (i=0; i<Naug; i++) {
          calc_i = prob(loci, r, i, j, loci[j+1][i], crosstype, 0);
          Ploci[i]*= calc_i;
        }
      }
    }
  } else {
//	Rprintf("FitQTL=Y\n");
    for (j=0; j<Nloci; j++) {
      for (i=0; i<Naug; i++) {
        Ploci[i]*= Pscale;
        Ploci[i+Naug]*= Pscale;
        Ploci[i+2*Naug]*= Pscale;
        // only for computational accuracy; see use of logP
      }
      if ((position[j]==MLEFT)||(position[j]==MUNLINKED)) {
        //Here we don't have any f2 dependancies anymore by using the prob function
        if (cofactor[j]<=MH)
          for (i=0; i<Naug; i++) {
            // calc_i= prob(loci, r, i, j, MH, crosstype, 0, 1);
            calc_i = start_prob(crosstype, loci[j][i]);
            Ploci[i] *= calc_i;
            Ploci[i+Naug] *= calc_i;
            Ploci[i+2*Naug] *= calc_i;
          }
        else
          for (i=0; i<Naug; i++) {
            //startvalues for each new chromosome
            Ploci[i]*= start_prob(crosstype, MAA);
            Ploci[i+Naug]*= start_prob(crosstype, MH);
            Ploci[i+2*Naug] *= start_prob(crosstype, MBB);
          }
        // QTL=MAA, MH orMBB
      }
      if ((position[j]==MLEFT)||(position[j]==MMIDDLE)) {
        if ((cofactor[j]<=MH)&&(cofactor[j+1]<=MH))
          for (i=0; i<Naug; i++) {
            // [pjotr:] was calc_i = prob(loci, r, i, j, loci[j+1][i], crosstype, 0, 0, 0); 
            // why define markertype if it is not used? FIXME
            calc_i = prob(loci, r, i, j, loci[j+1][i], crosstype, 0);
            Ploci[i]*= calc_i;
            Ploci[i+Naug]*= calc_i;
            Ploci[i+2*Naug]*= calc_i;
          }
        else if (cofactor[j]<=MH) // locus j+1 == QTL
          for (i=0; i<Naug; i++) { // QTL==MAA What is the prob of finding an MAA at J=1
            calc_i = prob(loci, r, i, j, MAA, crosstype, 0);
            Ploci[i]*= calc_i;
            // QTL==MH
            calc_i = prob(loci, r, i, j, MH, crosstype, 0);
            Ploci[i+Naug]*= calc_i;
            // QTL==MBB
            calc_i = prob(loci, r, i, j, MBB, crosstype, 0);
            Ploci[i+2*Naug]*= calc_i;
          }
        else // locus j == QTL
          for (i=0; i<Naug; i++) { // QTL==MAA
            calc_i = prob(loci, r, i, j+1, MAA, crosstype, -1);
            Ploci[i]*= calc_i;
            // QTL==MH
            calc_i = prob(loci, r, i, j+1, MH, crosstype, -1);
            Ploci[i+Naug]*= calc_i;
            // QTL==MBB
            calc_i = prob(loci, r, i, j+1, MBB, crosstype, -1);
            Ploci[i+2*Naug]*= calc_i;
          }
      }
    }
  }
//	Rprintf("INFO: Done fitting QTL's\n");
  if ((*weight)[0]== -1.0) {
    for (i=0; i<Nind; i++) indweight[i]= 0.0;
    if (fitQTL=='n') {
      for (i=0; i<Naug; i++) indweight[ind[i]]+=Ploci[i];
      for (i=0; i<Naug; i++) (*weight)[i]= Ploci[i]/indweight[ind[i]];
    } else {
      for (i=0; i<Naug; i++) indweight[ind[i]]+=Ploci[i]+Ploci[i+Naug]+Ploci[i+2*Naug];
      for (i=0; i<Naug; i++) {
        (*weight)[i]       = Ploci[i]/indweight[ind[i]];
        (*weight)[i+Naug]  = Ploci[i+Naug]/indweight[ind[i]];
        (*weight)[i+2*Naug]= Ploci[i+2*Naug]/indweight[ind[i]];
      }
    }
  }
  //Rprintf("Weights done\n");
  //Rprintf("Individual->trait->cofactor->weight\n");
  //for (int j=0; j<Nind; j++){
  //  Rprintf("%d->%f, %d, %f %f\n", j, y[j], cofactor[j], (*weight)[j], Ploci[j]);
  //}
  double logL=0;
  vector indL;
  indL= newvector(Nind);
  while ((iem<em)&&(delta>1.0e-5)) {
    iem+=1;
    if (varknown=='n') *variance=-1.0;
    //Rprintf("Checkpoint_b\n");
    logL= regression(Nind, Nloci, cofactor, loci, y,
                     weight, ind, Naug, variance, Fy, biasadj, fitQTL, dominance);
    logL=0.0;
    //Rprintf("regression ready\n");
    for (i=0; i<Nind; i++) indL[i]= 0.0;
    if (fitQTL=='n') // no QTL fitted
      for (i=0; i<Naug; i++) {
        (*weight)[i]= Ploci[i]*Fy[i];
        indL[ind[i]]= indL[ind[i]] + (*weight)[i];
      }
    else // QTL moved along the chromosomes
      for (i=0; i<Naug; i++) {
        (*weight)[i]= Ploci[i]*Fy[i];
        (*weight)[i+Naug]  = Ploci[i+Naug]*  Fy[i+Naug];
        (*weight)[i+2*Naug]= Ploci[i+2*Naug]*Fy[i+2*Naug];
        indL[ind[i]]+=(*weight)[i]+(*weight)[i+Naug]+(*weight)[i+2*Naug];
      }
    for (i=0; i<Nind; i++) logL+=log(indL[i])-logP;
    for (i=0; i<Nind; i++) indweight[i]= 0.0;
    if (fitQTL=='n') {
      for (i=0; i<Naug; i++) indweight[ind[i]]+=(*weight)[i];
      for (i=0; i<Naug; i++) (*weight)[i]/=indweight[ind[i]];
    } else {
      for (i=0; i<Naug; i++)
        indweight[ind[i]]+=(*weight)[i]+(*weight)[i+Naug]+(*weight)[i+2*Naug];
      for (i=0; i<Naug; i++) {
        (*weight)[i]       /=indweight[ind[i]];
        (*weight)[i+Naug]  /=indweight[ind[i]];
        (*weight)[i+2*Naug]/=indweight[ind[i]];
      }
    }
    delta= fabs(logL-oldlogL);
    oldlogL= logL;
  }
  //Rprintf("EM Finished\n");
  // bias adjustment after finished ML estimation via EM
  if ((useREML)&&(varknown=='n')) {
    // RRprintf("Checkpoint_c\n");
    *variance=-1.0;
    biasadj='y';
    logL= regression(Nind, Nloci, cofactor, loci, y,
                     weight, ind, Naug, variance, Fy, biasadj, fitQTL, dominance);
    logL=0.0;
    for (int _i=0; _i<Nind; _i++) indL[_i]= 0.0;
    if (fitQTL=='n')
      for (i=0; i<Naug; i++) {
        (*weight)[i]= Ploci[i]*Fy[i];
        indL[ind[i]]+=(*weight)[i];
      }
    else
      for (i=0; i<Naug; i++) {
        (*weight)[i]= Ploci[i]*Fy[i];
        (*weight)[i+Naug]= Ploci[i+Naug]*Fy[i+Naug];
        (*weight)[i+2*Naug]= Ploci[i+2*Naug]*Fy[i+2*Naug];
        indL[ind[i]]+=(*weight)[i];
        indL[ind[i]]+=(*weight)[i+Naug];
        indL[ind[i]]+=(*weight)[i+2*Naug];
      }
    for (i=0; i<Nind; i++) logL+=log(indL[i])-logP;
    for (i=0; i<Nind; i++) indweight[i]= 0.0;
    if (fitQTL=='n') {
      for (i=0; i<Naug; i++) indweight[ind[i]]+=(*weight)[i];
      for (i=0; i<Naug; i++) (*weight)[i]/=indweight[ind[i]];
    } else {
      for (i=0; i<Naug; i++) {
        indweight[ind[i]]+=(*weight)[i];
        indweight[ind[i]]+=(*weight)[i+Naug];
        indweight[ind[i]]+=(*weight)[i+2*Naug];
      }
      for (i=0; i<Naug; i++) {
        (*weight)[i]       /=indweight[ind[i]];
        (*weight)[i+Naug]  /=indweight[ind[i]];
        (*weight)[i+2*Naug]/=indweight[ind[i]];
      }
    }
  }
  for (i=0; i<Nind; i++){
    //Rprintf("IND %d Ploci: %f Fy: %f UNLOG:%f LogL:%f LogL-LogP: %f\n", i, Ploci[i], Fy[i], indL[i], log(indL[i]), log(indL[i])-logP);
  }
  Free(Fy);
  Free(Ploci);
  Free(indweight);
  Free(indL);
  return logL;
}

