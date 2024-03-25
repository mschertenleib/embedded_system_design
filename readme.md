# Assigment SW2 Part 2
Gilles Regamey 296642
Matieu Schertenleib 313318

## modified files
- all files in module/grayscale/verilog/*.v
- grayscale.c 
- project.files
- or1420SingleCore.v 

## ANSWERS
**Assignment 1:** *Record these numbers for the unmodified grayscale code.*
- Cycles: ~30'690'000 
- Stall: ~19'010'00 
- Bus Idle: 16'930'00  
- Effective Cyles: 11'680'329

**Assignment 2:** *Modify your grayscale code such that it uses the custom instruction.*

We made the custom instruction accept either 1 pixel or 4 at a time. Use `#define SINGLEPX` or `#define PARALLEL` in order to compile the according version.

**Assignment 3:** *Record again the above numbers for the execution of the custom-instruction accelerated grayscale code.*\

**For Singlepixel :**
 - Cycles: ~24'490'000
 - Stall: ~18'340'000
 - Bus Idle: ~11'550'000 
 - Effective cycles: 6'144'008

 **For batch processing :**
 - Cycles: ~9'720'000
 - Stall: ~8'260'000
 - Bus Idle: ~3'490'000 
 - Effective cycles: 1'459'207

**Assignment 4:** *Compare the above values and take your conclusions.*

Unsurprizingly, accelerating with hardware reduced the number of effective cycles needed for grayscale conversion. We get twice as fast with single pixel conversion and more than ten times faster with batch processing. We also see that the bus is less overwhelmed in batch processing as we get the four pixels in two batch of uint32 and write them to the grayscale buffer in one batch instead of the one to one processing of unoptimized and singlepixel processing. In batch we spend the majority of the time in stall, we think its waiting on memory access as the ratio Stall/Cycles is becomes larger if the processing time approaches the memory fetching time. 