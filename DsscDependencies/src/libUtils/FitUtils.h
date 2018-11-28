#ifndef FITUTILS_H
#define FITUTILS_H

#include "Minuit2/FCNGradientBase.h"
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/FunctionMinimum.h"
#include "gsl/gsl_errno.h"

#include <vector>
#include <tuple>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <math.h>
#include <assert.h>
#include "utils.h"

#include <boost/numeric/ublas/exception.hpp>

namespace utils{


//Pixel,yOffs,xFactor,xOffs,yOffsError,xFactorError,xOffsError
using ADCGainFitResult = std::array<double,7>;
using ADCGainFitResultsVec = std::vector<ADCGainFitResult>;
using ADCGainFitResultsMap = std::map<uint32_t,ADCGainFitResult>;

ADCGainFitResultsVec generateADCGainFitResults(const std::vector<double> & dataVector, const std::vector<uint32_t> & irampSettings);
ADCGainFitResult fitADCGainVector(std::vector<double> &rmpFineTrmVec, std::vector<double> &gainRatioVec );

std::vector<utils::Stats> calcMeanRMSADCGainFitParams(const ADCGainFitResultsVec &adcGainFitResultsVec, int &numGoodValues);

namespace ADCGainFitResults{
  static constexpr int NUMVALS = 7;
  void printHeader(std::ofstream & out, const std::string & pixelsStr);
  std::vector<std::string> getFitParamNames();
}


double gauss(const double *x, const double *par);
double gauss(double x, void *par);

double hyperbel(const double *x, const double *par);
double hyperbel(double x, void *par);
double inverseHyperbel(double y, const double *par);

double fitfunction(const double *x, const double *par);
double fitfunction(double x, void *par);

double fitfunctionSimplified(const double *x, const double *par);
double fitfunctionSimplified(double x, void *par);

ROOT::Minuit2::FunctionMinimum minuitFit(
    double * bins,
    double * vals,
    double * errs,
    int length,
    ROOT::Minuit2::MnUserParameters upar,
    double(*fitfunc)(double, void*) = &fitfunctionSimplified);

class GaussChi2FCN : public ROOT::Minuit2::FCNBase {

public:

  GaussChi2FCN(double * meas,
               double * pos,
               double * mvar,
               int length,
               double (*fitfunc)(double, void*) = &fitfunctionSimplified);
  ~GaussChi2FCN();

  double operator()(const std::vector<double>& par) const;
  double Up() const;

private:
  //std::vector<double> fMeasurements;
  //std::vector<double> fPositions;
  //std::vector<double> fMVariances;
  double * fMeasurements;
  double * fPositions;
  double * fMVariances;
  int dataLength;
  double (*m_fitfunc)(double, void*);
};


class GSLError {

public:

   GSLError() {
         gsl_set_error_handler(&GSLError::Handler);
      // set a new handler for GSL
   }

   static void Handler(const char * reason, const char * file, int line, int gsl_errno)  {

      //Error("++++GSLError","Error %d in %s at %d : %s",gsl_errno,file,line,reason);
     std::cout << "##### MY GSLError in line " << line << ":" << reason << std::endl;
   }
};

template<class T>
  class matrix
  {
  public:
    matrix(unsigned int nRows, unsigned int nCols) :
      m_nRows( nRows ),
      m_nCols( nCols ),
      m_oData( nRows*nCols, 0 )
    {
      if ( !nRows || !nCols )
      {
        throw std::range_error( "invalid matrix size" );
      }
    }

    static matrix identity( unsigned int nSize )
    {
      matrix oResult( nSize, nSize );

      int nCount = 0;
      std::generate( oResult.m_oData.begin(), oResult.m_oData.end(),
        [&nCount, nSize]() { return !(nCount++%(nSize + 1)); } );

      return oResult;
    }

    inline T& operator()(unsigned int nRow, unsigned int nCol)
    {
      if ( nRow >= m_nRows || nCol >= m_nCols )
      {
        throw std::out_of_range( "position out of range" );
      }

      return m_oData[nCol+m_nCols*nRow];
    }

    inline matrix operator*(matrix& other)
    {
      if ( m_nCols != other.m_nRows )
      {
        throw std::domain_error( "matrix dimensions are not multiplicable" );
      }

      matrix oResult( m_nRows, other.m_nCols );
      for ( unsigned int r = 0; r < m_nRows; ++r )
      {
        for ( unsigned int ocol = 0; ocol < other.m_nCols; ++ocol )
        {
          for ( unsigned int c = 0; c < m_nCols; ++c )
          {
            oResult(r,ocol) += (*this)(r,c) * other(c,ocol);
          }
        }
      }

      return oResult;
    }

    inline matrix transpose()
    {
      matrix oResult( m_nCols, m_nRows );
      for ( unsigned int r = 0; r < m_nRows; ++r )
      {
        for ( unsigned int c = 0; c < m_nCols; ++c )
        {
          oResult(c,r) += (*this)(r,c);
        }
      }
      return oResult;
    }

    inline unsigned int rows()
    {
      return m_nRows;
    }

    inline unsigned int cols()
    {
      return m_nCols;
    }

    inline std::vector<T> data()
    {
      return m_oData;
    }

    void print()
    {
      for ( unsigned int r = 0; r < m_nRows; r++ )
      {
        for ( unsigned int c = 0; c < m_nCols; c++ )
        {
          std::cout << (*this)(r,c) << "\t";
        }
        std::cout << std::endl;
      }
    }

  private:
    unsigned int m_nRows;
    unsigned int m_nCols;

    std::vector<T> m_oData;
  };

  template<typename T>
    class Givens
    {
    public:
      Givens() : m_oJ(2,2), m_oQ(1,1), m_oR(1,1)
      {
      }

      /*
        Calculate the inverse of a matrix using the QR decomposition.

        param:
          A	matrix to inverse
      */
      const matrix<T> Inverse( matrix<T>& oMatrix )
      {
        if ( oMatrix.cols() != oMatrix.rows() )
        {
          throw std::domain_error( "matrix has to be square" );
        }
        matrix<T> oIdentity = matrix<T>::identity( oMatrix.rows() );
        Decompose( oMatrix );
        return Solve( oIdentity );
      }

      /*
        Performs QR factorization using Givens rotations.
      */
      void Decompose( matrix<T>& oMatrix )
      {
        int nRows = oMatrix.rows();
        int nCols = oMatrix.cols();


        if ( nRows == nCols )
        {
          nCols--;
        }
        else if ( nRows < nCols )
        {
          nCols = nRows - 1;
        }

        m_oQ = matrix<T>::identity(nRows);
        m_oR = oMatrix;

        for ( int j = 0; j < nCols; j++ )
        {
          for ( int i = j + 1; i < nRows; i++ )
          {
            GivensRotation( m_oR(j,j), m_oR(i,j) );
            PreMultiplyGivens( m_oR, j, i );
            PreMultiplyGivens( m_oQ, j, i );
          }
        }

        m_oQ = m_oQ.transpose();
      }

      /*
        Find the solution for a matrix.
        http://en.wikipedia.org/wiki/QR_decomposition#Using_for_solution_to_linear_inverse_problems
      */
      matrix<T> Solve( matrix<T>& oMatrix )
      {
        matrix<T> oQtM( m_oQ.transpose() * oMatrix );
        int nCols = m_oR.cols();
        matrix<T> oS( 1, nCols );
        for (int i = nCols-1; i >= 0; i-- )
        {
          oS(0,i) = oQtM(i, 0);
          for ( int j = i + 1; j < nCols; j++ )
          {
            oS(0,i) -= oS(0,j) * m_oR(i, j);
          }
          oS(0,i) /= m_oR(i, i);
        }

        return oS;
      }

      const matrix<T>& GetQ()
      {
        return m_oQ;
      }

      const matrix<T>& GetR()
      {
        return m_oR;
      }

    private:
      /*
        Givens rotation is a rotation in the plane spanned by two coordinates axes.
        http://en.wikipedia.org/wiki/Givens_rotation
      */
      void GivensRotation( T a, T b )
      {
        T t,s,c;
        if (b == 0)
        {
          c = (a >=0)?1:-1;
          s = 0;
        }
        else if (a == 0)
        {
          c = 0;
          s = (b >=0)?-1:1;
        }
        else if (abs(b) > abs(a))
        {
          t = a/b;
          s = -1/sqrt(1+t*t);
          c = -s*t;
        }
        else
        {
          t = b/a;
          c = 1/sqrt(1+t*t);
          s = -c*t;
        }
        m_oJ(0,0) = c; m_oJ(0,1) = -s;
        m_oJ(1,0) = s; m_oJ(1,1) = c;
      }

      /*
        Get the premultiplication of a given matrix
        by the Givens rotation.
      */
      void PreMultiplyGivens( matrix<T>& oMatrix, int i, int j )
      {
        int nRowSize = oMatrix.cols();

        for ( int nRow = 0; nRow < nRowSize; nRow++ )
        {
          double nTemp = oMatrix(i,nRow) * m_oJ(0,0) + oMatrix(j,nRow) * m_oJ(0,1);
          oMatrix(j,nRow) = oMatrix(i,nRow) * m_oJ(1,0) + oMatrix(j,nRow) * m_oJ(1,1);
          oMatrix(i,nRow) = nTemp;
        }
      }

    private:
      matrix<T> m_oJ, m_oQ, m_oR;
    };

    template<typename T>
      std::vector<T> polyfit( const std::vector<T>& oX, const std::vector<T>& oY, int nDegree )
      {
        if ( oX.size() != oY.size() )
          throw std::invalid_argument( "X and Y vector sizes do not match" );

        // more intuative this way
        nDegree++;

        size_t nCount =  oX.size();
        matrix<T> oXMatrix( nCount, nDegree );
        matrix<T> oYMatrix( nCount, 1 );

        // copy y matrix
        for ( size_t i = 0; i < nCount; i++ )
        {
          oYMatrix(i, 0) = oY[i];
        }

        // create the X matrix
        for ( size_t nRow = 0; nRow < nCount; nRow++ )
        {
          T nVal = 1.0f;
          for ( int nCol = 0; nCol < nDegree; nCol++ )
          {
            oXMatrix(nRow, nCol) = nVal;
            nVal *= oX[nRow];
          }
        }

        // transpose X matrix
        matrix<T> oXtMatrix( oXMatrix.transpose() );
        // multiply transposed X matrix with X matrix
        matrix<T> oXtXMatrix( oXtMatrix * oXMatrix );
        // multiply transposed X matrix with Y matrix
        matrix<T> oXtYMatrix( oXtMatrix * oYMatrix );

        Givens<T> oGivens;
        oGivens.Decompose( oXtXMatrix );
        matrix<T> oCoeff = oGivens.Solve( oXtYMatrix );
        // copy the result to coeff
        return oCoeff.data();
      }

      /*
        Calculates the value of a polynomial of degree n evaluated at x. The input
        argument pCoeff is a vector of length n+1 whose elements are the coefficients
        in incremental powers of the polynomial to be evaluated.

        param:
          oCoeff			polynomial coefficients generated by polyfit() function
          oX				x axis values

        return:
          Fitted Y values. C++0x-compatible compilers make returning locally
          created vectors very efficient.
      */
      template<typename T>
      std::vector<T> polyval( const std::vector<T>& oCoeff, const std::vector<T>& oX )
      {
        size_t nCount =  oX.size();
        size_t nDegree = oCoeff.size();
        std::vector<T>	oY( nCount );

        for ( size_t i = 0; i < nCount; i++ )
        {
          T nY = 0;
          T nXT = 1;
          T nX = oX[i];
          for ( size_t j = 0; j < nDegree; j++ )
          {
            // multiply current x by a coefficient
            nY += oCoeff[j] * nXT;
            // power up the X
            nXT *= nX;
          }
          oY[i] = nY;
        }

        return oY;
      }
}

#endif // FITUTILS_H
