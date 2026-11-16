#!/bin/bash

function summarize() {
  dir=$1; use=$2
  outfile="${dir}/stats_${use}.txt"
  hostname > ${outfile}
  echo ${use} >> ${outfile}

  # echo -e "# Time" >> ${outfile}
  for file in `find ${dir} -maxdepth 1 -type f -name "*_*.txt"`; do
    if [[ ${file} =~ "stats" ]]; then
      continue
    fi
    time=`cat ${file} | egrep '(Elapsed )?[T|t]ime'`
    line=`echo -n ${time} | sed -e "s/\(Elapsed \)\?[T|t]ime \+= \([0-9]\+\.[0-9]\+\)\( seconds\)\?/,\2/g"`
    echo ${line:1} | tr -d ' ' >> ${outfile}
  done

  echo -e "\n# Abort" >> ${outfile}
  for file in `find ${dir} -maxdepth 1 -type f -name "*_*.txt"`; do
    if [[ ${file} =~ "stats" ]]; then
      continue
    fi
    grep_str="# HTMTxConflict *:"
    if [[ ${dir} =~ "tiny" ]]; then
      grep_str="#aborts *:"
    fi
    abort=`cat ${file} | egrep "${grep_str}"`
    line=`echo -n "${abort}" | sed -e "s/${grep_str}/,/g"`
    line=`echo ${line} | tr -d ' '`
    echo ${line:1} >> ${outfile}
  done
}

if [[ $# = "0" ]]; then return; fi

summarize $1 $2
