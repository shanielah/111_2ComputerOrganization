// TODO :    
"vsetvli t0, a0, e16, m1, ta, ma \n\t" 
"vle16.v v0, (%[p_x]) \n\t"         // v0 = p_x[j=0~7]

"vle16.v v6, (%[p_tmp2]) \n\t"   
"vadd.vx v7, v6, %[target] \n\t"    // fulfill with target

"addi t1, zero, 8 \n\t" 
"addi t2, zero, 1 \n\t"                      
"addi t4, zero, -1 \n\t"

"L1: \n\t"
"beq %[flag], t2, E1 \n\t"      // if flag == 1, then break the whole loop

"lh t5, 0(%[p_x]) \n\t"              
"vadd.vx v1, v0, t5 \n\t"       //  v1 = p_x[i] + p_x[j=0~7]
"vmseq.vv v2, v1, v7 \n\t"       

"vfirst.m t3, v2 \n\t"          // if v2 has the value 1 then writes that element’s index to t3, else -1 is written to t3
"beq t3, t4, NotFound \n\t"
"addi %[flag], zero, 1 \n\t"

"NotFound: \n\t"
"addi t1, t1, -1 \n\t"  
"addi %[p_x], %[p_x], 2 \n\t"       
    
"bne t1, zero, L1 \n\t"            
"E1: \n\t"