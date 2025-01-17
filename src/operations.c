/**
 * @file operations.c
 * @brief All the mathematical functions for general or symmetrical
 *        matrices, with real or complex coefficients.
 * 
 */
#include "lanczos.h"
#include "operations.h"

void OP_transpose(Matrix *M)
{
  assert(M->n_cols == M->n_rows);
  switch (M->type)
  {
    case General:
      if(M->isComplex == true)
      {
        double a;
        double b;
        for(unsigned int i = 0; i < M->n_rows; i++)
        {
          for (unsigned int j = i + 1; j < M->n_cols; j++)
          {
            a = M->array_real[i*M->n_cols + j];
            b = M->array_imag[i*M->n_cols + j];
            M->array_real[i*M->n_cols + j] = M->array_real[j*M->n_cols + i];
            M->array_imag[i*M->n_cols + j] = M->array_imag[j*M->n_cols + i];
            M->array_real[j*M->n_cols + i] = a;
            M->array_imag[j*M->n_cols + i] = b;
          }
        }
      }
      else
      {
        double a;
        for(unsigned int i = 0; i < M->n_rows; i++)
        {
          for (unsigned int j = i + 1; j < M->n_cols; j++)
          {
            a = M->array_real[i*M->n_cols + j];
            M->array_real[i*M->n_cols + j] = M->array_real[j*M->n_cols + i];
            M->array_real[j*M->n_cols + i] = a;
          }
        }
      }
      break;
    case Symmetric:
      break;
    default:
      break;
  }
}

// "High level"
Vector OP_MatrixVector(Matrix *M, Vector *v)
{
  Vector result;
  switch (M->type)
  {
    case General:
      if(M->isComplex)
      {
        result = OP_Complex_General_matrixVector(M,v);
      }
      else
      {
        result = OP_Real_General_matrixVector(M,v);
      }
      break;
    case Symmetric:
      if(M->isComplex)
      {
        // TO-DO
        printf("Not implemented yet !");
        exit(1);
      }
      else
      {
        result = OP_Real_Symmetric_matrixVector_V3(M,v);
      }
      break;
  default:
    break;
  }
  return result;
}


// "Low Level"

// Real part

// Real + General y = A*v
Vector OP_Real_General_matrixVector(Matrix* M, Vector *v)
{
  Vector y;
  y = createVector(v->n, false);
  allocateVector(&y);
  #pragma omp parallel for
  {
    for (size_t i = 0; i < M->n_rows; i++)
    {
      y.array_real[i] = 0;
      for(size_t j = 0; j < M->n_cols; j++)
      {
        y.array_real[i] += M->array_real[i*M->n_cols + j] * v->array_real[j];
      }
      
    }
  }
  return y;
}

// Real + Symmetric y = A*v
Vector OP_Real_Symmetric_matrixVector(Matrix* M, Vector *v)
{
  Vector y;
  y = createVector(v->n, false);
  allocateVector(&y);

  int* shift = (int*)malloc(sizeof(int) * M->n_rows);
  shift[0] = 0;
  y.array_real[0] = M->array_real[shift[0]] * v->array_real[0];
  for (size_t i = 1 ; i < M->n_rows; i++)
  {
    shift[i] = shift[i - 1] + (M->n_cols - i + 1);
    y.array_real[i] = M->array_real[shift[i]] * v->array_real[i];
  }
  
  #pragma omp parallel for
  {
    for (size_t i = 0; i < M->n_rows; i++)
    {
      const int index = shift[i] - i;
      for (size_t j = i+1 ; j < M->n_cols; j++)
      {
        y.array_real[i] += M->array_real[index + j] * v->array_real[j];
      }
    }
  }
  for (size_t i = 0; i < M->n_rows; i++)
  {
    const int index = shift[i] - i;
    #pragma omp parallel for
    {
      for (size_t j = i+1 ; j < M->n_cols; j++)
      {
        y.array_real[j] += M->array_real[index + j] * v->array_real[i];
      }
    }
  }
  free(shift);
  return y;
}

// Real + Symmetric y = A*v
Vector OP_Real_Symmetric_matrixVector_V2(Matrix* M, Vector *v)
{
  Vector y;
  y = createVector(v->n, false);
  allocateVector(&y);
  double* L_tmp = (double*)calloc(v->n, sizeof(double));
  int* shift = (int*)malloc(sizeof(int) * M->n_rows);
  shift[0] = 0;
  y.array_real[0] = M->array_real[shift[0]] * v->array_real[0];
  for (size_t i = 1 ; i < M->n_rows; i++)
  {
    shift[i] = shift[i - 1] + (M->n_cols - i + 1);
    y.array_real[i] = M->array_real[shift[i]] * v->array_real[i];
  }
  
  #pragma omp parallel for reduction(+:L_tmp[:M->n_rows])
  {
    for (size_t i = 0; i < M->n_rows; i++)
    {
      const int index = shift[i] - i;
      for (size_t j = i+1 ; j < M->n_cols; j++)
      {
        y.array_real[i] += M->array_real[index + j] * v->array_real[j];
        L_tmp[j] += M->array_real[index +j] * v->array_real[i];
      }
    }
  }
  #pragma omp parallel for
  {
    for (size_t i = 0; i < M->n_rows; i++)
    {
      y.array_real[i] += L_tmp[i];
    }
  }
  free(L_tmp);
  free(shift);
  return y;
}

// Real + Symmetric y = A*v
Vector OP_Real_Symmetric_matrixVector_V3(Matrix* M, Vector *v)
{
  Vector y;
  y = createVector(v->n, false);
  allocateVector(&y);
  double* L_tmp = (double*)calloc(v->n, sizeof(double));
  int* shift = (int*)malloc(sizeof(int) * M->n_rows);
  shift[0] = 0;
  y.array_real[0] = M->array_real[shift[0]] * v->array_real[0];
  for (size_t i = 1 ; i < M->n_rows; i++)
  {
    shift[i] = shift[i - 1] + (M->n_cols - i + 1);
    y.array_real[i] = M->array_real[shift[i]] * v->array_real[i];
  }
  
  #pragma omp parallel for reduction(+:L_tmp[:M->n_rows])
  {
    for (size_t i = 0; i < M->n_rows; i++)
    {
      const int index = shift[i] - i;
      #pragma omp simd
      for (size_t j = i+1 ; j < M->n_cols; j++)
      {
        y.array_real[i] += M->array_real[index + j] * v->array_real[j];
      }
      #pragma unroll 4
      for (size_t j = i+1 ; j < M->n_cols; j++)
      {
        L_tmp[j] += M->array_real[index +j] * v->array_real[i];
      }
    }
  }
  #pragma omp parallel for simd
  {
    for (size_t i = 0; i < M->n_rows; i++)
    {
      y.array_real[i] += L_tmp[i];
    }
  }
  free(L_tmp);
  free(shift);
  return y;
}


void OP_Real_scaleVector(Vector *v, double constReal)
{
  #pragma omp parallel for simd
  {
    for (size_t i = 0; i < v->n; i++)
    {
      v->array_real[i] = constReal * v->array_real[i];
    }
  }
  return;
}

Vector OP_Real_AddVector(Vector *x, Vector *y)
{ 
  assert(x->n == y->n);
  Vector v;
  v = createVector(x->n, false);
  allocateVector(&v);
  #pragma omp parallel for simd
  {
    for (size_t i = 0; i < v.n; i++)
    {
      v.array_real[i] = x->array_real[i] + y->array_real[i];
    }
  }
  return v;
}

double OP_Real_DotProduct(Vector *x, Vector *y)
{ 
  assert(x->n == y->n);
  double result = 0.0; 
  #pragma omp parallel for reduction(+ : result)
  {
    for (size_t i = 0; i < x->n; i++)
    {
      result += x->array_real[i] * y->array_real[i];
    }
  }
  return result;
}

double OP_Real_Norm(Vector *v)
{ 
  double result = 0.0;
  result = OP_Real_DotProduct(v,v);
  result = sqrt(result);
  return result;
} 

// Complex Part

/*
z3 = z1*z2
z1 = a + b*i
z2 = c + d*i
z3 = (ac-bd) + i(ad + bc)
*/

// Complex + General y = A*v
Vector OP_Complex_General_matrixVector(Matrix* M, Vector *v)
{
  Vector y;
  y = createVector(v->n, true);
  allocateVector(&y);
  #pragma omp parallel for
  {
    for (size_t i = 0; i < M->n_rows; i++)
    {
      y.array_real[i] = 0;
      y.array_imag[i] = 0;
      for(size_t j = 0; j < M->n_cols; j++)
      {
        y.array_real[i] += (M->array_real[i*M->n_cols + j] * v->array_real[j] - M->array_imag[i*M->n_cols + j] * v->array_imag[j]);
        y.array_imag[i] += (M->array_real[i*M->n_cols + j] * v->array_imag[j] - M->array_imag[i*M->n_cols + j] * v->array_real[j]);
      }
    }
  }
  return y;
}

double DotProduct(double *x, double *y,int vectorSize)
{
  double result = 0.0;

  #pragma omp parallel for reduction(+ : result)
    for (int i = 0; i < vectorSize; i++)
    {
      result += x[i] * y[i];
    }
  return result;
}

double NormVector(double *v,int vectorSize)
{
  double result = 0.0;
  result = DotProduct(v,v,vectorSize);
  result = sqrt(result);
  return result;
}
