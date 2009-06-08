/**********************************************************************
 * 
 * MQMscan.h
 *
 * copyright (c) 2009 Danny Arends
 * last modified Apr, 2009
 * first written Feb, 2009
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
 * C functions for the R/qtl package
 * Contains: R_scanMQM, scanMQM
 *
 **********************************************************************/

 /* Test interface  intended to only calls mapQTL with 1 trait using precalculated data */
void R_testScan(int *num_ind, int *num_markers, int *num_genotypes,double *trait,double *markerdistances,int *geno,double *probabilitymatrix, double *markerrf,int *cof,int *stepsize,int *windowsize);

int mqm(int Nind, int Nmark, int nGeno, vector y, vector mapdistance, cvector mappositions, imatrix Geno, Mmatrix Prob, 
			  matrix r, ivector cofactor, ivector selcofactor, double windowsize,int stepsize,char fitQTL,char dominance,char REMLorML);

/**********************************************************************
 * 
 * R_scanMQM
 * Wrapper for call from R;
 * 
 **********************************************************************/

void R_scanMQM(int *Nind,int *Nmark,int *Npheno,
			   int *geno,int *chromo, double *dist, double *pheno, 
			   int *cofactors, int *backwards, int *RMLorML,double *alfa,int *emiter,
			   double *windowsize,double *steps,
			   double *stepmi,double *stepma, int *nRun,int *out_Naug,int *indlist,  double *qtl,int *reestimate,int *crosstype,int *domi,int *verbose);


/**********************************************************************
 * 
 * scanMQM
 *
 * 
 **********************************************************************/

void scanMQM(int Nind, int Nmark,int Npheno,int **Geno,int **Chromo, 
			 double **Dist, double **Pheno, int **Cofactors, int Backwards, int RMLorML,double Alfa,int Emiter,
			 double Windowsize,double Steps,
			 double Stepmi,double Stepma,int NRUN,int out_Naug,int **INDlist, double **QTL, int re_estimate,int crosstype,int domi,int verbose);

/**********************************************************************
 * 
 * Helper functions
 *
 *
 **********************************************************************/


double Lnormal(double residual, double variance);
double absdouble(double x);
int mod(int a, int b);
/**********************************************************************
void reorg_geno(int n_ind, int n_pos, int *geno, int ***Geno);
*/
void reorg_pheno(int n_ind, int n_mar, double *pheno, double ***Pheno);

void reorg_int(int n_ind, int n_mar, int *pheno, int ***Pheno);


/* end of scanMQM.h */
