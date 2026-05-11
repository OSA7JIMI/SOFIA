LOG=v1/log_out
TT=v1/total_time.txt
grep "Total Time:" $LOG* | awk -v OFMT=%.17g '{sum+=$NF} END {print "Total Time: " sum}' > $TT