INPUT_PATH=$1
OUTPUT_PATH=$2

echo "Symbol,Address,Size,Object" > ${OUTPUT_PATH}
awk '/^ .text/ {if ($2 != "" && $2 >= "0x00200000") print $1 "," $2 "," $3 "," $4}' ${INPUT_PATH} >> ${OUTPUT_PATH}
awk '/^ .text/ {if ($2 == "") {A=$1;getline;A=A "," $1 "," $2 "," $3;if($1 >= "0x00200000") print A;A=""}}' ${INPUT_PATH} >> ${OUTPUT_PATH}
awk '/^ .data/ {if ($2 != "" && $2 >= "0x00200000") print $1 "," $2 "," $3 "," $4}' ${INPUT_PATH} >> ${OUTPUT_PATH}
awk '/^ .data/ {if ($2 == "") {A=$1;getline;A=A "," $1 "," $2 "," $3;if($1 >= "0x00200000") print A;A=""}}' ${INPUT_PATH} >> ${OUTPUT_PATH}
awk '/^ .rodata/ {if ($2 != "" && $2 >= "0x00200000") print $1 "," $2 "," $3 "," $4}' ${INPUT_PATH} >> ${OUTPUT_PATH}
awk '/^ .rodata/ {if ($2 == "") {A=$1;getline;A=A "," $1 "," $2 "," $3;if($1 >= "0x00200000") print A;A=""}}' ${INPUT_PATH} >> ${OUTPUT_PATH}
awk '/^ \*\* merge strings/ {A=$1" "$2" "$3;getline;A=A "," $1 "," $2;print A}' ${INPUT_PATH} >> ${OUTPUT_PATH}
awk '/^.got/ {print $1 "," $2 "," $3}' ${INPUT_PATH} >> ${OUTPUT_PATH}
echo "End of Profiling..."
