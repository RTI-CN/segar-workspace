#!/usr/bin/bash

_SETUP_PATH=$(cd `dirname $BASH_SOURCE[0]`; pwd)
USER_HOME_DIR=$(getent passwd $USER | cut -d: -f6)

SEGARRT_PATH=$(dirname "$_SETUP_PATH")

PATH=$_SETUP_PATH/third_party/bin:$PATH
LD_LIBRARY_PATH=$_SETUP_PATH/third_party/lib:$LD_LIBRARY_PATH

SEGAR_PATH=$_SETUP_PATH
export PATH  LD_LIBRARY_PATH  SEGAR_PATH
# create log dir
SEGAR_LOG_DIR_PREFIX="$USER_HOME_DIR/.segar/log"
if [ ! -d "$SEGAR_LOG_DIR_PREFIX" ]; then
    mkdir -p "$SEGAR_LOG_DIR_PREFIX"
fi
export USR_MSG_PATH=$_SETUP_PATH/msg_libs
export GLOG_log_dir="$SEGAR_LOG_DIR_PREFIX"
export GLOG_alsologtostderr=1
export GLOG_colorlogtostderr=1
export GLOG_minloglevel=0
export sysmo_start=0
export SEGAR_DOMAIN_ID=0
export SEGAR_IP=127.0.0.1

source $_SETUP_PATH/third_party/segar_cli_auto_complete.bash
