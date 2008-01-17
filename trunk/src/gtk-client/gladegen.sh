BUFFERNAME="`echo ${1} | sed -e 's:.*/::' -e 's:\..*::'`_glade_definition"
P="`echo ${0} | sed 's:\(.*/\)\([^/]*\)$:\1:'`"
FILE="`echo ${1} | sed \"s:$P::\"`"
echo -en "/**\n * NOTE: This file is autogenerated, DO NOT EDIT THIS FILE,\n * EDIT ${FILE} INSTEAD!\n */\n\n" > ${2}
echo "char ${BUFFERNAME}[] =" >> ${2}
cat ${1} | sed s:\\\\:\\\\\\\\:g | sed s:\":\\\\\":g | sed s:$:\": | sed s:^:\": >> ${2}
echo ";" >> ${2}
echo "int ${BUFFERNAME}_size = sizeof(${BUFFERNAME});" >> ${2}
