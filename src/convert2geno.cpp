#include <Rcpp.h>
using namespace Rcpp;

#include "convert2geno.h"

// [[Rcpp::export]]
IntegerMatrix cpp_convert2geno(const List xodat, const NumericVector map, const IntegerMatrix founder_geno)
{
    int n_ind = xodat.size();
    int n_mar = map.size();
    IntegerMatrix matmatrix(n_mar, n_ind), patmatrix(n_mar, n_ind);

    // size of founder geno matrix
    // (ignored if 0)
    int size_founder_geno = founder_geno.nrow() * founder_geno.ncol();

    for(int i=0; i<n_ind; i++) {
        List ind = xodat[i];
        List mat = ind[0];
        List pat = ind[1];

        // convert mat and pat chr to genotype matrices
        IntegerVector matdata = convertchr2geno(mat, map);
        std::copy(matdata.begin(), matdata.end(), matmatrix.begin()+i*n_mar);
        IntegerVector patdata = convertchr2geno(pat, map);
        std::copy(patdata.begin(), patdata.end(), patmatrix.begin()+i*n_mar);
    }

    // find maximum genotype
    int max_geno_mat = max(matmatrix);
    int max_geno_pat = max(patmatrix);
    int max_geno = max_geno_mat > max_geno_pat ? max_geno_mat : max_geno_pat;

    if(size_founder_geno > 0 && founder_geno.nrow() >= max_geno) {
        // convert to SNP genotypes using founder genotypes
        return combine_mat_and_pat_geno_wfounders(matmatrix, patmatrix, founder_geno);
    }
    else {
        // convert to 1/2/3 or binary code
        return combine_mat_and_pat_geno(matmatrix, patmatrix, max_geno);
    }
}


IntegerVector convertchr2geno(const List chr, const NumericVector map)
{
    IntegerVector alleles = chr[0];
    NumericVector locations = chr[1];

    int n_mar = map.size();
    int n_loc = locations.size();

    IntegerVector output(n_mar);

    // at each marker, find closest position to right and take allele there
    for(int i=0; i<n_mar; i++) {
        for(int j=0; j<n_loc; j++) {
            if(locations[j] >= map[i]) {
                output[i] = alleles[j];
                break;
            }
        }
    }

    return output;
}

IntegerMatrix combine_mat_and_pat_geno(const IntegerMatrix matmatrix, const IntegerMatrix patmatrix, const int max_geno)
{
    int n_mar = matmatrix.nrow();
    int n_ind = matmatrix.ncol();
    int n = n_mar * n_ind;
    IntegerMatrix result(n_mar, n_ind);

    if(max_geno == 2) {
        for(int i=0; i<n; i++)
            result[i] = matmatrix[i] + patmatrix[i] - 1;
    }
    else {  // multi-allele case
        for(int i=0; i<n; i++)
            result[i] = (1 << (matmatrix[i]-1)) + (1 << (patmatrix[i] - 1));
    }
    
    return result;
}

IntegerMatrix combine_mat_and_pat_geno_wfounders(const IntegerMatrix matmatrix, const IntegerMatrix patmatrix, const IntegerMatrix founder_geno)
{
    int n_mar = matmatrix.nrow();
    int n_ind = matmatrix.ncol();
    IntegerMatrix result(n_mar, n_ind);

    for(int i=0; i<n_mar; i++)
        for(int j=0; j<n_ind; j++)
            result(i,j) = founder_geno( matmatrix(i,j) - 1, i) + 
                founder_geno( patmatrix(i,j) - 1, i) - 1;

    return result;
}

