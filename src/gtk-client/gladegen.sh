BUFFERNAME="`echo ${1} | sed -e 's:.*/::' -e 's:\..*::'`_glade_definition"
echo "char ${BUFFERNAME}[] =" >> ${2}
cat ${1} | sed s:\\\\:\\\\\\\\:g | sed s:\":\\\\\":g | sed s:$:\": | sed s:^:\": >> ${2}
echo ";" >> ${2}
echo "int ${BUFFERNAME}_size = sizeof(${BUFFERNAME});" >> ${2}

