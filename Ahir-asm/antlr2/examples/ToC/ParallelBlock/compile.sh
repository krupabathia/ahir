../../../bin/Aa2C ParallelBlock.aa
indent aa_c_model.c
indent aa_c_model.h
gcc -g -c -I../../../include -I./ aa_c_model.c
gcc -g -c -I../../../include -I./ driver.c
gcc -g -o driver driver.o aa_c_model.o
rm *.o *~
