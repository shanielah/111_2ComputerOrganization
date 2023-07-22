#include <stdio.h>
int main()
{
    int i=0;
    int h[9]={0}, x[3]={0}, y[3]={0}; 
    FILE *input = fopen("../input/4.txt","r");
    for(i = 0; i<9; i++) fscanf(input, "%d", &h[i]);
    for(i = 0; i<3; i++) fscanf(input, "%d", &x[i]);
    for(i = 0; i<3; i++) fscanf(input, "%d", &y[i]);
    fclose(input);
    int *p_x = &x[0] ;
    int *p_h = &h[0] ;
    int *p_y = &y[0] ;

    int x0, f, val_x, val_h, val_y, three = 3;

    asm volatile(
        "andi %[i], %[i], 0\n\t"     // i=0
        "addi %[x0], %[p_x],0\n\t"

        "L1:\n\t"
        "addi %[p_x], %[x0], 0\n\t"   // p_x = &x[0]
        "andi %[f], %[f], 0\n\t"     // f=0
        
        "L2:\n\t"
        "lw %[val_h], 0(%[p_h])\n\t"
        "lw %[val_x], 0(%[p_x])\n\t"
        "lw %[val_y], 0(%[p_y])\n\t"

        "mul %[val_h], %[val_h], %[val_x]\n\t"
        "add %[val_y], %[val_y], %[val_h]\n\t"
        "sw %[val_y], 0(%[p_y])\n\t"

        "addi %[p_h], %[p_h], 4\n\t"
        "addi %[p_x], %[p_x], 4\n\t"
        "addi %[f], %[f], 1\n\t"

        "blt %[f], %[three], L2\n\t"

        "addi %[p_y], %[p_y], 4\n\t"
        "addi %[i], %[i], 1\n\t"

        "blt %[i], %[three], L1\n\t"
        : [val_x] "+r"(val_x), [val_h] "+r"(val_h), [val_y] "+r"(val_y),
          [p_x] "+r"(p_x), [p_h] "+r"(p_h), [p_y] "+r"(p_y),
          [i] "+r"(i), [f] "+r"(f), [x0] "+r"(x0)
        : [three] "r"(three)

        /*Your Code*/);

    p_y = &y[0];
    for (i = 0; i < 3; i++)
        printf("%d \n", *p_y++);
    return (0);
}