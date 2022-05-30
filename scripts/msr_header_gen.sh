#!/bin/bash

# MUST BE RUN AS ROOT

# Focusing on Intel for the moment.  As this is for testing variorum, we will
# use external tools.

# Assume for the moment that msr values read from cpu 0 will be identical to
# all other cpus.

# First range of MSRs:  0x0000 - 0x4000 (0-8192)

#for msr in `seq 0 8192`

# Actual range for Intel "architectural" msrs is 0x0 - 0x17DA and
# 0xC0000000 to 0xC0000103, per April 2022 edition of SDM v4.
MIN_MSR_SEQ_A=0			# 0x0
#MAX_MSR_SEQ_A=8192		# 0x2000
MAX_MSR_SEQ_A=8

MIN_MSR_SEQ_B=3221225472	# 0xC0000000
#MAX_MSR_SEQ_B=3221225984	# 0xC0000200
MAX_MSR_SEQ_B=3221225484

VENDOR_ID=` cat /proc/cpuinfo | grep "vendor_id"              | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
MODEL_NAME=`cat /proc/cpuinfo | grep "model name"             | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
CPU_FAMILY=`cat /proc/cpuinfo | grep "cpu family"             | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
CPU_MODEL=` cat /proc/cpuinfo | grep "model" | grep -v "name" | uniq | cut -d ":" -f "2" | sed -e "s/ //"`
CPU_STEP=`  cat /proc/cpuinfo | grep "stepping"               | uniq | cut -d ":" -f "2" | sed -e "s/ //"`

HEADERFILE=`printf "./%02x%02x_msr_samples.h" $CPU_FAMILY $CPU_MODEL`
touch ${HEADERFILE}
exec 1>$HEADERFILE	# https://ops.tips/gists/redirect-all-outputs-of-a-bash-script-to-a-file/



echo "/******************************************************************************"
echo " * "
echo " * " Sample MSR values from an ${MODEL_NAME}.
echo -n " * "
printf " Architecture:  %02x%02x\n" $CPU_FAMILY $CPU_MODEL
echo " * " File generated on ${DATE}.
echo " * "
echo " *****************************************************************************/"
echo
echo "#ifndef MSR_SAMPLES_H"
echo "#define MSR_SAMPLES_H"
echo "#define VENDOR_ID  \"${VENDOR_ID}\""
echo "#define MODEL_NAME \"${MODEL_NAME}\""
echo "#define CPU_FAMILY $CPU_FAMILY"
echo "#define CPU_MODEL  $CPU_MODEL"
echo "#define CPU_STEP   $CPU_STEP"
echo
echo "#define MIN_MSR_SEQ_A $MIN_MSR_SEQ_A"
echo "#define MAX_MSR_SEQ_A $MAX_MSR_SEQ_A"
echo "#define MIN_MSR_SEQ_B $MIN_MSR_SEQ_B"
echo "#define MAX_MSR_SEQ_B $MAX_MSR_SEQ_B"
echo
echo "struct msr_tuple{ bool is_valid, unsigned long long val };"
echo
echo "struct msr_tuple msr_table_A[] = {"

for msr in `seq ${MIN_MSR_SEQ_A} ${MAX_MSR_SEQ_A}`
do
	if MSR_VAL=`rdmsr --c-lang --zero-pad --processor 0 ${msr} 2>/dev/null`; then
		echo -e '\t' `printf "/*msr id %#06x */" ${msr}` " { /*is valid*/ 1, /*value*/" ${MSR_VAL}"ULL},"
	else
		echo -e '\t' `printf "/*msr id %#06x */" ${msr}` " { /*is valid*/ 0, /*value*/ 0ULL},"
	fi
done

echo "};"

echo "struct msr_tuple msr_table_B[] = {"

# FIXME Massive copy-paste.  Fix it later.
for msr in `seq ${MIN_MSR_SEQ_B} ${MAX_MSR_SEQ_B}`
do
	if MSR_VAL=`rdmsr --c-lang --zero-pad --processor 0 ${msr} 2>/dev/null`; then
		echo -e '\t' `printf "/*msr id %#010x */" ${msr}` " { /*is valid*/ 1, /*value*/" ${MSR_VAL}"ULL},"
	else
		echo -e '\t' `printf "/*msr id %#010x */" ${msr}` " { /*is valid*/ 0, /*value*/ 0ULL},"
	fi
done

echo "};"
echo "#endif /*MSR_SAMPLES_H*/"

