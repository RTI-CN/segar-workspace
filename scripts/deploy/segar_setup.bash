#!/usr/bin/bash

_SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")"; pwd)
_SEGAR_SDK_PATH=$(cd "$_SCRIPT_DIR/.."; pwd)
_PROJ_PATH=$(cd "$_SEGAR_SDK_PATH/.."; pwd)
_COMMUNICATIONS_PATH=$(cd "$_PROJ_PATH/communications"; pwd)

LD_LIBRARY_PATH=$_SEGAR_SDK_PATH/lib:$LD_LIBRARY_PATH
PATH=$_SEGAR_SDK_PATH/bin:$PATH
PYTHONPATH=$_SEGAR_SDK_PATH/lib/python3.10/site-packages:$_COMMUNICATIONS_PATH/lib/python:$PYTHONPATH
SEGAR_MSG_LIB_PATH=$_SEGAR_SDK_PATH/share/msg_libs
SEGAR_PATH=$_SCRIPT_DIR
SEGAR_GLOBAL_PATH=$_SCRIPT_DIR
export PATH LD_LIBRARY_PATH PYTHONPATH SEGAR_PATH SEGAR_GLOBAL_PATH SEGAR_MSG_LIB_PATH
# create log dir
_USER_HOME_DIR="$(getent passwd "$(id -un)" | cut -d: -f6 | tr -d '\r')"
_SEGAR_LOG_DIR_PREFIX="$_USER_HOME_DIR/.segar/log"
if [ ! -d "$_SEGAR_LOG_DIR_PREFIX" ]; then
    mkdir -p "$_SEGAR_LOG_DIR_PREFIX"
fi
export GLOG_log_dir="$_SEGAR_LOG_DIR_PREFIX"
export GLOG_alsologtostderr=1
export GLOG_colorlogtostderr=1
export GLOG_minloglevel=0
export sysmo_start=0
export SEGAR_DOMAIN_ID=0
export SEGAR_IP=127.0.0.1

source "$_SCRIPT_DIR/segar_cli_auto_complete.bash"
