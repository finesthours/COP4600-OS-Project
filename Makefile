###########################################################                    
#                                                         #
#  Makefile COP 4600 Project --- Do NOT modify this file  #
#                                                         #
#                  Spring 2010 Version                    #
#                                                         #
###########################################################                    

SHELL = /bin/sh
COMMON = /home/eurip/ossim2010
CC = gcc
CFLAGS = -g -I$(COMMON)
LFLAGS = -lm 
CCF = $(CC) $(CFLAGS)
INCLUDES = $(COMMON)/externs.h $(COMMON)/osdefs.h

RM = /bin/rm -f
CP = /bin/cp -f

all: sim

sim: obj1.o obj2.o obj3.o obj4.o obj5.o obj6.o $(COMMON)/simulator.o
	rm -f sim
	$(CCF) -o sim obj[1-6].o $(COMMON)/simulator.o $(LFLAGS)

$(COMMON)/simulator.o: $(COMMON)/simulator.c $(INCLUDES)
	$(CCF) -c -g -o $(COMMON)/simulator.o $(COMMON)/simulator.c

obj1.o: obj1.c $(INCLUDES)
	$(CCF) -c obj1.c

obj2.o: obj2.c $(INCLUDES)
	$(CCF) -c obj2.c

obj3.o: obj3.c $(INCLUDES)
	$(CCF) -c obj3.c

obj4.o: obj4.c $(INCLUDES)
	$(CCF) -c obj4.c

obj5.o: obj5.c $(INCLUDES)
	$(CCF) -c obj5.c

obj6.o: obj6.c $(INCLUDES)
	$(CCF) -c obj6.c

data1files: 
	$(CP) $(COMMON)/data/1/* .

data2files: 
	$(CP) $(COMMON)/data/2/* .

data3files: 
	$(CP) $(COMMON)/data/3/* .

data4files: 
	$(CP) $(COMMON)/data/4/* .

data5files: 
	$(CP) $(COMMON)/data/5/* .

data6files: 
	$(CP) $(COMMON)/data/6/* .

submit:
	
	@echo
	@echo "  Submitting..."
	@echo
	@echo "  Make sure you are in your own directory and that all"
	@echo "  your obj[#].c files are present when executing"
	@echo "  'make submit'."
	@echo
	@$(COMMON)/submit
	@echo "  Hooray! Submission seems to have succeeded."
	@echo

compare: sim
	@echo Comparing results for Objective ${OBJ}...
	@echo Running your program ....
	@./sim >/dev/null
	@echo Running ossim ...
	@$(COMMON)/ossim >/dev/null
	@echo Stripping the headers..
	@sed -e 1,13d <ossim.out >temp
	@tr -s '\012' '\012' <temp >temp1
	@sed -e 1,13d <data${OBJ}.out >temp
	@tr -s '\012' '\012' <temp >temp2
	@echo Comparing the outputs...
	@diff -w temp1 temp2
	@/bin/rm temp1 temp2 temp
	@echo
	@echo "  Hooray! Your output matches the gold standard. Good work!"
	@echo

correct-output:
	$(COMMON)/ossim

copyfiles:
	$(CP) $(COMMON)/*.c $(COMMON)/*.h $(COMMON)/intro.doc .

clean: 
	$(RM) obj[1-6].o sim
