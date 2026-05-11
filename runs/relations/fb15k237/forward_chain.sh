# Parameter filename
PARAMS=p_fb.txt

case "$1" in
    v1) versions=(1) ;;
    v2) versions=(2) ;;
    v3) versions=(3) ;;
    v4) versions=(4) ;;
    all|"") versions=(1 2 3 4) ;;
    *)
        echo "Invalid argument: $1. Choose from: v1, v2, v3, v4, all"
        exit 1
        ;;
esac

for i in "${versions[@]}"
do
# write data path to file
DATAPATH="data_directory ../../../data/inductivefb15k237/v$i"
echo >> $PARAMS
echo $DATAPATH >> $PARAMS

# Input rules filename
INPUTRULES=v$i/rules_all.txt
# Log filename
LOG=v$i/log_$OUTNAME
# Forward chain filename
FORWARDCHAIN=v$i/induced_facts.txt
# Executable
EXEC=../../../code/lprules

$EXEC -p $PARAMS -v $INPUTRULES -f $FORWARDCHAIN > $LOG-forward_chain.txt &

wait

sed -i '$d' $PARAMS

done