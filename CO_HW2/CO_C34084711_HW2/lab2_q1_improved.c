// TODO : 
"addi s1, zero, 16 \n\t"  // input size n = 16
"addi %[arith_cnt], %[arith_cnt], 1 \n\t" 

"L2:  \n\t"

"vsetvli t0, s1, e16, m1, ta, ma \n\t"  //t0 for the elements in one vector operation, s1 for total elements to be processed
"addi %[others_cnt], %[others_cnt], 1 \n\t"

"vle16.v v0, (%[h]) \n\t"  
"addi %[lw_cnt], %[lw_cnt], 1 \n\t"

"sub s1, s1, t0 \n\t"  
"addi %[arith_cnt], %[arith_cnt], 1 \n\t" 

"slli t0, t0, 1 \n\t"  // 1 short int = 2 bytes
"addi %[arith_cnt], %[arith_cnt], 1 \n\t" 

"add %[h], %[h], t0 \n\t"   
"addi %[arith_cnt], %[arith_cnt], 1 \n\t" 

"vle16.v v1, (%[x]) \n\t" 
"addi %[lw_cnt], %[lw_cnt], 1 \n\t"

"add %[x], %[x], t0 \n\t"  
"addi %[arith_cnt], %[arith_cnt], 1 \n\t" 

"vadd.vv v2, v0, v1 \n\t"   
"addi %[arith_cnt], %[arith_cnt], 1 \n\t" 

"vse16.v v2, (%[y]) \n\t"  
"addi %[sw_cnt], %[sw_cnt], 1 \n\t" 

"add %[y], %[y], t0 \n\t"   
"addi %[arith_cnt], %[arith_cnt], 1 \n\t" 

"addi %[others_cnt], %[others_cnt], 1 \n\t"
"bne s1, zero, L2 \n\t"
