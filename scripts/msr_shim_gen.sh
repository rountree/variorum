#!/bin/bash

# MUST BE RUN AS ROOT
# Requires bash 4.3 or later.
# h/t to https://unix.stackexchange.com/questions/545502/bash-array-of-arrays

# Output:  a file xxyy_msr_samples.h where xx is the cpu family and yy is
# the cpu model (both in hexidecimal).

# Note:  If you change this file be sure to run the astyle check script
# on the output.

# Iterates over address ranges of model-specific registers determining which
# are valid, and of those, their current value.

# Uses the stock msr kernel module and Intel's msr-tools.  (This could be
# rewritten using Variorum easily enough, but as the results will be used
# in testing Variorum it's probably best to use an independent toolchain.)

# Only samples from a single CPU.

################################################################################
#
# Desribe ranges of MSR addresses
#
################################################################################

# Actual range for Intel "architectural" msrs is 0x0 - 0x17DA and
# 0xC0000000 to 0xC0000103, per April 2022 edition of SDM v4.

# Intel uses two very disjoint sequences of MSR addresses.
declare -a SEQUENCES=(SEQ_A SEQ_B)

# SEQA 0x0 0x2000
declare -a SEQ_A=(0 8192)

# SEQB 0xC0000000 0xC0000200
declare -a SEQ_B=(3221225472 3221225984)

#MIN_MSR_SEQ_A=0			# 0x0
#MAX_MSR_SEQ_A=8192		# 0x2000

#MIN_MSR_SEQ_B=3221225472	# 0xC0000000
#MAX_MSR_SEQ_B=3221225984	# 0xC0000200

################################################################################
#
# Collect cpu information
#
################################################################################

VENDOR_ID=` cat /proc/cpuinfo | grep "vendor_id"              | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
MODEL_NAME=`cat /proc/cpuinfo | grep "model name"             | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
CPU_FAMILY=`cat /proc/cpuinfo | grep "cpu family"             | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
CPU_MODEL=` cat /proc/cpuinfo | grep "model" | grep -v "name" | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
CPU_STEP=`  cat /proc/cpuinfo | grep "stepping"               | uniq | cut -d ":" -f "2" | sed -e "s/ //"`

HEADERFILE=`printf "./%02x%02x_msr_samples.h" $CPU_FAMILY $CPU_MODEL`
touch ${HEADERFILE}
exec 1>$HEADERFILE	# https://ops.tips/gists/redirect-all-outputs-of-a-bash-script-to-a-file/

################################################################################
#
# Create the header file.
#
################################################################################

echo "/******************************************************************************"
echo " *"
echo " * Generated automatically.  DO NOT EDIT (unless you know what you're doing)."
echo " *"
echo " * Sample MSR values from an ${MODEL_NAME}."
echo -n " *"
printf " Architecture:  %02x%02x\n" $CPU_FAMILY $CPU_MODEL
echo " * Compile with -std=c99 or better."
echo " * Assumes sizeof(long long)==8."
echo " * File generated on " `date`
echo " *"
echo " *****************************************************************************/"
echo
echo "#ifndef MSR_SAMPLES_H"
echo "#define MSR_SAMPLES_H"
echo "#include <stdbool.h>"
echo "static const char *VENDOR_ID  = \"${VENDOR_ID}\";"
echo "static const char *MODEL_NAME = \"${MODEL_NAME}\";"
echo "static const unsigned char CPU_FAMILY = $CPU_FAMILY;"
echo "static const unsigned char CPU_MODEL  = $CPU_MODEL;"
echo "static const unsigned char CPU_STEP   = $CPU_STEP;"
echo

for group in "${SEQUENCES[@]}";
do
	declare -n lst="$group"
	echo "static const unsigned long long MIN_MSR_$group = ${lst[0]}ULL;"
	echo "static const unsigned long long MAX_MSR_$group = ${lst[1]}ULL;"
done

echo
echo "struct msr_tuple"
echo "{"
echo "    unsigned long long address;"
echo "    unsigned long long val1;"
echo "    unsigned long long val2;"
echo "    bool is_valid;"
echo "};"
echo
echo "static struct msr_tuple msr_tuple[] ="
echo "{"

for group in "${SEQUENCES[@]}";
do
	declare -n lst="$group"
	for msr in `seq "${lst[0]}" "${lst[1]}"`;
	do
		if MSR_VAL1=`rdmsr --c-lang --zero-pad --processor 0 ${msr} 2>/dev/null`; then
			MSR_VAL2=`rdmsr --c-lang --zero-pad --processor 0 ${msr} 2>/dev/null`
			VALID=1
		else
			MSR_VAL1=0
			MSR_VAL2=0
			VALID=0
		fi
		printf "    { %#010x, %#018x, %#018x, %d },\n" ${msr} ${MSR_VAL1} ${MSR_VAL2} ${VALID}
	done
done
echo "};"
echo "#endif /*MSR_SAMPLES_H*/"

