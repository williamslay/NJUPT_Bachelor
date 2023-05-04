export SUBJECT=/home/will/software_test/NJUPT_Work/NJUPT-Bachelor/testing/test
export TMP_DIR=/home/will/software_test/NJUPT_Work/NJUPT-Bachelor/testing/temp

pushd $SUBJECT
make clean
make
popd

cat $TMP_DIR/BBnames.txt | grep -v "^$"| rev | cut -d: -f2- | rev | sort | uniq > $TMP_DIR/BBnames2.txt && mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt
cat $TMP_DIR/BBcalls.txt | grep -Ev "^[^,]*$|^([^,]*,){2,}[^,]*$"| sort | uniq > $TMP_DIR/BBcalls2.txt && mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt
cat $TMP_DIR/BBcallcounts.txt | grep -Ev "^[^,]*$|^([^,]*,){2,}[^,]*$"| sort | uniq -c| awk '{print $2","$1}'> $TMP_DIR/BBcallcounts2.txt && mv $TMP_DIR/BBcallcounts2.txt $TMP_DIR/BBcallcounts.txt

for file in $TMP_DIR/dot-files/*.dot; do
    filename=$(basename "$file")
    dot -Tpng "$file" -o "$TMP_DIR/dot-files/${filename%.*}.png"
done       