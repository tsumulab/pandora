#!/bin/bash -ex

Progs="genome intruder kmeans labyrinth ssca2 vacation yada bayes"
STMs=("norec" "swissTM" "TinySTM" "tl2")
TMs=("norec" "swisstm" "tinystm" "tl2" "rtm")


args=()
do_build=false
while (( $# > 0 )); do
  case $1 in
    -h | --help)
      echo "./sbuilder.sh <backend> <program> [<out_dir>]"
      exit 0
      ;;
    -b | --build)
      do_build=true
      ;;
    -d)
      src_dir=$2
      shift
      ;;
    -* | --*)
      echo "No such option : $1"
      exit 1
      ;;
    *)
      args+=($1)
      ;;
  esac
  shift
done


: "Check format" && {
  len=${#args[@]}
  if [[ ${len} != '2' ]] && [[ ${len} != '3' ]]; then
    echo "./sbuilder.sh <backend> <program> [<out_dir>]"
    exit 1
  fi
}


use=${args[0]}
bench=${args[1]}
if [[ ${use} =~ "lock" ]] || [[ ${use} = "seq" ]] || [[ ${use} = "rtm" ]]; then
  : # nothing
else
  stm=${use}
  if [[ ${use} = "tinystm" ]]; then
    stm="tinySTM-1.0.5"
  fi
fi

(cd lib && rm -f *.o || true)
(rm -f lib/tm.h common/Makefile)

if [[ -n "${stm}" ]] && [[ ${do_build} = "true" ]]; then
  echo -n "Building ${stm} ... "
  (
    cd ${stm}
    make clean >&/dev/null
    make >&/dev/null
    if [[ $? != 0 ]]; then
      echo -e "\nBuild error : ${stm}"
      exit 1
    fi
  )
  echo "done"
fi

if [[ -z "${src_dir}" ]]; then
  src_dir=${bench}
fi

echo -n "Compiling ${bench} ... "
(
  cp backends/${use}/tm.h lib/tm.h
  cp backends/${use}/Makefile common/Makefile

  cd ${src_dir}
  make clean
  make #1> /dev/null
  rc=$?
  if [[ $rc != 0 ]] ; then
    echo -e "\nBuild error : ${bench}"
    rm *.o ${bench} || true
    exit 1
  fi
  echo "done"

  dst=${use}
  if [[ ${len} = '3' ]]; then
    dst=${args[2]}
  fi

  mkdir -p "../build/${dst}"
  if [[ -f ../build/${dst}/${bench} ]]; then
    rm -f ../build/${dst}/${bench} >&/dev/null
  fi
  cp ./${bench} ../build/${dst}

  make clean
)

(cd lib && rm -f *.o || true)
(rm -f lib/tm.h common/Makefile)
