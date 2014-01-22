import os
import stat
import math
import re

# Calculates mean of list
# ----------------------------------------------------------------------------------------------------

def mean(values):
	cnt = 0
	acu = 0
	
	for v in values:
		acu = acu+v
		cnt = cnt + 1
	
	acu = acu/cnt
	
	return acu

# ----------------------------------------------------------------------------------------------------


# Calculates standard deviation of list
# ----------------------------------------------------------------------------------------------------

def desv(values,mean):
	cnt = 0
	acu = 0
	acu2 = 0
	
	for v in values:
		acu2 = v-mean
		acu2 = acu2*acu2
		cnt = cnt + 1
	
	acu = acu2/cnt
	
	acu = math.sqrt(acu)
	
	return acu

# ----------------------------------------------------------------------------------------------------


# range for float
# ----------------------------------------------------------------------------------------------------

def drange(start, stop, step):
	r = start
	interval = [r]
	while r < stop:
		r += step
		interval.append(r)
		
	return interval
		
# ----------------------------------------------------------------------------------------------------


# Main program
# ---------------------------------------------------------------------------------------------------

# Initial conditions

wr_sender = 'eth0'
period = 1000000
size_p = 1400
npack = 1000
bursts = 10
kill = 0

ntest = 0

wr_sender_log_file = 'log.tst'

# CSV file name (add .csv)

csv_file = 'test'

# Test Configuration file

test_config_file = 'test.cfg'

# Global Configuration file

global_config_file = 'global.cfg'

# Read and process global config file

try:
	fglobalconfig = open(global_config_file,'r')

	for param in fglobalconfig:

		param_s = param.split()

		comentario = re.match('\s*#.*',param) or re.match('\n.*',param) or param_s == []
	
		if not comentario:
			param_name = param_s[0]
   
			if param_name == 'CSV_FILE':
				csv_file = param_s[1]
			elif param_name == 'TEST_FILE':
				test_config_file = param_s[1]
			elif param_name == 'RECEIVER_START':
				if param_s[1] == 'REMOTE': # on debugging
					os.system('sudo scp'+' '+'./wr-receiver '+param_s[2]+'@'+param_s[3]+':wr-receiver')
					os.system('sudo ssh'+' '+param_s[2]+'@'+param_s[3]+' '+'killall wr-receiver')
					os.system('sudo ssh'+' '+param_s[2]+'@'+param_s[3]+' '+'./wr-receiver '+param_s[4]+' 0 '+' &')
				elif param_s[1] == 'LOCAL':
					os.system('killall wr-receiver')
					os.system('./wr-receiver'+' '+param_s[2]+' 1 '+' &')
				else:
					print('Error: RECEIVER_START option unrecognized')
			else:
				print('Error: '+param+' Parameter unrecognized')

	fglobalconfig.close()
	
except:
	print('\n Global config file not found: Defaults values will apply...')

# Read test config file to configure tests

try:
	cfile = open(test_config_file,'r')
except:
	print('\n Test config file not found: Finishing script...')
	os.exit(1)

# Executes each test

test_file = cfile.readlines()
total_test = len(test_file)

for test in test_file:

	# Initialize localhost flag

	localhost = 1

	# Extract arguments of test
	
	test_arg = test.split()

	comentario = re.match('\s*#.*',test) or re.match('\n.*',test) or test_arg == []

	if not comentario:
	
		# Remove TEST

		test_arg = test_arg[1:]
	
		# Load test variable
	
		param = test_arg[0]
	
		if param == 'SIZE':
			variable_param = 'SIZE'
			vinitial = int(test_arg[1])
			vinc = int(test_arg[2])
			vend = int(test_arg[3])
		elif param == 'PERIOD':
			variable_param = 'PERIOD'
			vinitial = int(test_arg[1])
			vinc = int(test_arg[2])
			vend = int(test_arg[3])
		elif param == 'NBURSTS':
			variable_param = 'NBURSTS'
			vinitial = int(test_arg[1])
			vinc = int(test_arg[2])
			vend = int(test_arg[3])
		elif param == 'NPACKG':
			variable_param = 'NPACKG'
			vinitial = int(test_arg[1])
			vinc = int(test_arg[2])
			vend = int(test_arg[3])
		elif param_name == 'RUN_REMOTE':
			hostname = test_arg[1]
			localhost = 0
		else:
			print('Error: '+param+' Parameter unrecognized')
		
		# Remove test variable
	
		test_arg = test_arg[4:]
	
		# Built range of test variable
	
		variable_range = drange(vinitial,vend,vinc)
	
		# Load test parameters
	
		index = 0
		end_test = len(test_arg)
	
		while index < end_test:
			param = test_arg[index];
		
			if param == 'SIZE':
				size_p = test_arg[index+1]
				index = index+2
			elif param == 'PERIOD':
				period = test_arg[index+1]
				index = index+2
			elif param == 'NBURSTS':
				bursts = test_arg[index+1]
				index = index+2
			elif param == 'NPACKG':
				npack = test_arg[index+1]
				index = index+2
			elif param == 'WR_ETH_SENDER':
				wr_sender = test_arg[index+1]
				index = index+2
			else:
				print('Error: '+param+' Parameter unrecognized')


		
		# Built CSV file with results
	
		fcsv = open(csv_file+str(ntest)+'.csv','w')
	
		fcsv.write(variable_param+', '+'BW Mbps (average), '+'BW Mbps (desv), '+'Lost packages (average), '+'Lost packages (desv), Corrupted packages (average), Corrupted packages (desv) \n')
	
	
		bw_average = []
		bw_desv = []
		lost_pkg_average = []
		lost_pkg_desv = []
		corrupted_pkg_average = []
		corrupted_pkg_desv = []

		index_value = 0
		end_value = len(variable_range)
	
		for value in variable_range:

			bandwidth = []
			lost_packages = []
			corrupted_packages = []

                	print('Prueba con '+str(value)+'\n')
	
			# Initialize and run process
                
			if variable_param == 'SIZE':
				wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+str(period)+' '+str(value)+' '+str(npack)+' '+str(bursts)+' '+str(kill)
				wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+str(period)+' '+'<size>'+' '+str(npack)+' '+str(bursts)+' '+str(kill)
				
				if localhost == 0:
					wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+hostname+' '+str(period)+' '+str(value)+' '+str(npack)+' '+str(bursts)+' '+str(kill)
					wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+hostname+' '+str(period)+' '+'<size>'+' '+str(npack)+' '+str(bursts)+' '+str(kill)

				print('./wr-sender'+' '+wr_sender+' '+str(period)+' '+str(value)+' '+str(npack)+' '+str(bursts)+' '+str(kill))
			elif variable_param == 'PERIOD':
				wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+str(value)+' '+str(size_p)+' '+str(npack)+' '+str(bursts)+' '+str(kill)
				wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+'<period>'+' '+str(size_p)+' '+str(npack)+' '+str(bursts)+' '+str(kill)

				if localhost == 0:
					wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+hostname+' '+str(value)+' '+str(size_p)+' '+str(npack)+' '+str(bursts)+' '+str(kill)
					wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+hostname+' '+'<period>'+' '+str(size_p)+' '+str(npack)+' '+str(bursts)+' '+str(kill)
				
				print('./wr-sender'+' '+wr_sender+' '+str(value)+' '+str(size_p)+' '+str(npack)+' '+str(bursts)+' '+str(kill))
			elif variable_param == 'NBURSTS':
				wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+str(period)+' '+str(size_p)+' '+str(npack)+' '+str(value)+' '+str(kill)
				wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+str(period)+' '+str(size_p)+' '+str(npack)+' '+'<nbursts>'+' '+str(kill)
				
				if localhost == 0:
					wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+hostname+' '+str(period)+' '+str(size_p)+' '+str(npack)+' '+str(value)+' '+str(kill)
					wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+hostname+' '+str(period)+' '+str(size_p)+' '+str(npack)+' '+'<nbursts>'+' '+str(kill)

				print('./wr-sender'+' '+wr_sender+' '+str(period)+' '+str(size_p)+' '+str(npack)+' '+str(value)+' '+str(kill))
			elif variable_param == 'NPACKG':
				wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+str(period)+' '+str(size_p)+' '+str(value)+' '+str(bursts)+' '+str(kill)
				wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+str(period)+' '+str(size_p)+' '+'<npackages>'+' '+str(bursts)+' '+str(kill)

				if localhost == 0:
					wr_sender_cmd = './wr-sender'+' '+wr_sender+' '+hostname+' '+str(period)+' '+str(size_p)+' '+str(value)+' '+str(bursts)+' '+str(kill)
					wr_sender_cmd_to_csv = './wr-sender'+' '+wr_sender+' '+hostname+' '+str(period)+' '+str(size_p)+' '+'<npackages>'+' '+str(bursts)+' '+str(kill)

				print('./wr-sender'+' '+wr_sender+' '+str(period)+' '+str(size_p)+' '+str(value)+' '+str(bursts)+' '+str(kill))

			os.system(wr_sender_cmd)

			# Process result file
		
                	fresults = open(wr_sender_log_file,'r')
	
                	# Extract results
		
                	init_result = 0
	
                	for result in fresults:
                        	result_arg = result.split()
                        	print(result_arg)

                        	if init_result == 1:
                                	bw = float(result_arg[1])
                                	lost_pkg = int(result_arg[2])
					corrupted_pkg = int(result_arg[3])
                                	bandwidth.append(bw)
                                	lost_packages.append(lost_pkg)
					corrupted_packages.append(corrupted_pkg)
			
                        	if result_arg[0] == 'VALUES':
                                	init_result = 1

			# Calculate bw, lost and corrupted packages in average
	
                	bw_a = mean(bandwidth)   
			lost_pkg_a = mean(lost_packages)
			corrupted_pkg_a = mean(corrupted_packages)
			
			# Add to average list
		
                	bw_average.append(bw_a)
                	lost_pkg_average.append(lost_pkg_a)
			corrupted_pkg_average.append(corrupted_pkg_a)

			# Calculate bw, lost and corrupted packages desviation

			bw_d = desv(bandwidth,bw_a)
			lost_pkg_d = desv(lost_packages,lost_pkg_a)
			corrupted_pkg_d = desv(corrupted_packages,corrupted_pkg_a)

			# Add to desviation list
			
                	bw_desv.append(bw_d)
                	lost_pkg_desv.append(lost_pkg_d)
			corrupted_pkg_desv.append(corrupted_pkg_d)
		
                	fresults.close()

			# Write into CSV file

			fcsv.write(str(value)+', '+str(bw_a)+', '+str(bw_d)+', '+str(lost_pkg_a)+', '+str(lost_pkg_d) +', '+str(corrupted_pkg_a)+', '+str(corrupted_pkg_d)+'\n')

			index_value = index_value+1
		
		fcsv.write('\n\n');
		fcsv.write('Comand executed:,'+wr_sender_cmd_to_csv+'\n');
		fcsv.close()

		os.chmod(csv_file+str(ntest)+'.csv',0666)
		
		ntest = ntest+1

# Close config file

cfile.close()
