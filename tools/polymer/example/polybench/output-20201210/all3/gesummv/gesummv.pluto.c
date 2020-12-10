#include <math.h>
#define ceild(n,d)  (((n)<0) ? -((-(n))/(d)) : ((n)+(d)-1)/(d))
#define floord(n,d) (((n)<0) ? -((-(n)+(d)-1)/(d)) : (n)/(d))
#define max(x,y)    ((x) > (y)? (x) : (y))
#define min(x,y)    ((x) < (y)? (x) : (y))

// TODO: mlir-clang %s %stdinclude | FileCheck %s
// RUN: clang %s -O3 %stdinclude %polyverify -o %s.exec1 && %s.exec1 &> %s.out1
// RUN: mlir-clang %s %polyverify %stdinclude -emit-llvm | clang -x ir - -O3 -o %s.execm && %s.execm &> %s.out2
// RUN: rm -f %s.exec1 %s.execm
// RUN: diff %s.out1 %s.out2
// RUN: rm -f %s.out1 %s.out2
// RUN: mlir-clang %s %polyexec %stdinclude -emit-llvm | clang -x ir - -O3 -o %s.execm && %s.execm > %s.mlir.time; cat %s.mlir.time | FileCheck %s --check-prefix EXEC
// RUN: clang %s -O3 %polyexec %stdinclude -o %s.exec2 && %s.exec2 > %s.clang.time; cat %s.clang.time | FileCheck %s --check-prefix EXEC
// RUN: rm -f %s.exec2 %s.execm

/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* gesummv.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "gesummv.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
		DATA_TYPE POLYBENCH_1D(x,N,n))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < n; i++)
    {
      x[i] = (DATA_TYPE)( i % n) / n;
      for (j = 0; j < n; j++) {
	A[i][j] = (DATA_TYPE) ((i*j+1) % n) / n;
	B[i][j] = (DATA_TYPE) ((i*j+2) % n) / n;
      }
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(y,N,n))

{
  int i;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("y");
  for (i = 0; i < n; i++) {
    if (i % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
    fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, y[i]);
  }
  POLYBENCH_DUMP_END("y");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gesummv(int n,
		    DATA_TYPE alpha,
		    DATA_TYPE beta,
		    DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		    DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
		    DATA_TYPE POLYBENCH_1D(tmp,N,n),
		    DATA_TYPE POLYBENCH_1D(x,N,n),
		    DATA_TYPE POLYBENCH_1D(y,N,n))
{
  int i, j;

  int t1, t2, t3, t4, t5;
 register int lbv, ubv;
if (_PB_N >= 1) {
  for (t2=0;t2<=floord(_PB_N-1,32);t2++) {
    for (t3=32*t2;t3<=min(_PB_N-1,32*t2+31);t3++) {
      y[t3] = SCALAR_VAL(0.0);;
    }
  }
  for (t2=0;t2<=floord(_PB_N-1,32);t2++) {
    for (t3=0;t3<=floord(_PB_N-1,32);t3++) {
      for (t4=32*t2;t4<=min(_PB_N-1,32*t2+31);t4++) {
        for (t5=32*t3;t5<=min(_PB_N-1,32*t3+31);t5++) {
          y[t4] = B[t4][t5] * x[t5] + y[t4];;
        }
      }
    }
  }
  for (t2=0;t2<=floord(_PB_N-1,32);t2++) {
    for (t3=32*t2;t3<=min(_PB_N-1,32*t2+31);t3++) {
      tmp[t3] = SCALAR_VAL(0.0);;
    }
  }
  for (t2=0;t2<=floord(_PB_N-1,32);t2++) {
    for (t3=0;t3<=floord(_PB_N-1,32);t3++) {
      for (t4=32*t2;t4<=min(_PB_N-1,32*t2+31);t4++) {
        for (t5=32*t3;t5<=min(_PB_N-1,32*t3+31);t5++) {
          tmp[t4] = A[t4][t5] * x[t5] + tmp[t4];;
        }
      }
    }
  }
  for (t2=0;t2<=floord(_PB_N-1,32);t2++) {
    for (t3=32*t2;t3<=min(_PB_N-1,32*t2+31);t3++) {
      y[t3] = alpha * tmp[t3] + beta * y[t3];;
    }
  }
}

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(tmp, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n, &alpha, &beta,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B),
	      POLYBENCH_ARRAY(x));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gesummv (n, alpha, beta,
		  POLYBENCH_ARRAY(A),
		  POLYBENCH_ARRAY(B),
		  POLYBENCH_ARRAY(tmp),
		  POLYBENCH_ARRAY(x),
		  POLYBENCH_ARRAY(y));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(y)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(tmp);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);

  return 0;
}

// CHECK: func @kernel_gesummv(%arg0: i32, %arg1: f64, %arg2: f64, %arg3: memref<1300x1300xf64>, %arg4: memref<1300x1300xf64>, %arg5: memref<1300xf64>, %arg6: memref<1300xf64>, %arg7: memref<1300xf64>) {
// CHECK-NEXT:  %cst = constant 0.000000e+00 : f64
// CHECK-NEXT:  %0 = index_cast %arg0 : i32 to index
// CHECK-NEXT:  affine.for %arg8 = 0 to %0 {
// CHECK-NEXT:    affine.store %cst, %arg5[%arg8] : memref<1300xf64>
// CHECK-NEXT:    affine.store %cst, %arg7[%arg8] : memref<1300xf64>
// CHECK-NEXT:    %1 = affine.load %arg5[%arg8] : memref<1300xf64>
// CHECK-NEXT:    %2 = affine.load %arg7[%arg8] : memref<1300xf64>
// CHECK-NEXT:    affine.for %arg9 = 0 to %0 {
// CHECK-NEXT:      %8 = affine.load %arg3[%arg8, %arg9] : memref<1300x1300xf64>
// CHECK-NEXT:      %9 = affine.load %arg6[%arg9] : memref<1300xf64>
// CHECK-NEXT:      %10 = mulf %8, %9 : f64
// CHECK-NEXT:      %11 = addf %10, %1 : f64
// CHECK-NEXT:      affine.store %11, %arg5[%arg8] : memref<1300xf64>
// CHECK-NEXT:      %12 = affine.load %arg4[%arg8, %arg9] : memref<1300x1300xf64>
// CHECK-NEXT:      %13 = affine.load %arg6[%arg9] : memref<1300xf64>
// CHECK-NEXT:      %14 = mulf %12, %13 : f64
// CHECK-NEXT:      %15 = addf %14, %2 : f64
// CHECK-NEXT:      affine.store %15, %arg7[%arg8] : memref<1300xf64>
// CHECK-NEXT:    }
// CHECK-NEXT:    %3 = affine.load %arg5[%arg8] : memref<1300xf64>
// CHECK-NEXT:    %4 = mulf %arg1, %3 : f64
// CHECK-NEXT:    %5 = affine.load %arg7[%arg8] : memref<1300xf64>
// CHECK-NEXT:    %6 = mulf %arg2, %5 : f64
// CHECK-NEXT:    %7 = addf %4, %6 : f64
// CHECK-NEXT:    affine.store %7, %arg7[%arg8] : memref<1300xf64>
// CHECK-NEXT:  }
// CHECK-NEXT:  return
// CHECK-NEXT: }

// EXEC: {{[0-9]\.[0-9]+}}
