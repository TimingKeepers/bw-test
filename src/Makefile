# --------------------------------------------------------------------------------------------------
# This makefile builds wr-sender and wr-receiver programs and allows to execute test (test-bw.py)
#
# Author: Miguel Jiménez López <klyone@correo.ugr.es>
#
# Supervisor: Javier Díaz Alonso <jda@ugr.es>
#
# University: University of Granada
#
# Version: 1.0
#
# Released to the public domain as sample code to be customized.
#
# It has been performed in framework of "Beca de la iniciación a la investigación" of University of Granada. 
#
# This program is inspired in wr-agent and wr-ruler Alessandro Rubbini's codes. <link>
#
#
# --------------------------------------------------------------------------------------------------
# Rules
# --------------------------------------------------------------------------------------------------
#
# all: Compile wr-sender and wr-receiver 
# wr-sender: Compile wr-sender
# wr-receiver: Compile wr-receiver
# test: Execute test with wr-sender and wr-receiver (Do not forget write configuration files!)
# clean: Delete wr-sender and wr-receiver program file
#
# --------------------------------------------------------------------------------------------------

all: wr-sender wr-receiver

wr-sender: wr-sender.c wr-rec-send.h
	@echo "Compiling wr-sender..."
	gcc -O3 -o wr-sender wr-sender.c -lrt
	@echo "Compilation is complete!"

wr-receiver: wr-receiver.c wr-rec-send.h
	@echo "Compiling wr-receiver..."
	gcc -O3 -o wr-receiver wr-receiver.c -lrt	
	@echo "Compilation is complete!"

test:
	@echo "Executing test..."
	python test-bw.py
	@echo "Test is complete!"

clean:
	@echo "Removing executable files..."
	rm -f wr-receiver wr-sender
	@echo "All files was deleted!"
