#!/usr/bin/bash

__SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")"; pwd)
__SEGAR_SDK_PATH=$(cd "$__SCRIPT_DIR/third_party"; pwd)

LD_LIBRARY_PATH=$__SEGAR_SDK_PATH/lib:$__SCRIPT_DIR/lib:$LD_LIBRARY_PATH
PATH=$__SEGAR_SDK_PATH/bin:$PATH
PYTHONPATH=$__SCRIPT_DIR/lib/python3.10/site-packages:$__SEGAR_SDK_PATH/lib/python3.10/site-packages:$PYTHONPATH
SEGAR_MSG_LIB_PATH=$__SCRIPT_DIR/msg_libs
SEGAR_PATH=$__SCRIPT_DIR
SEGAR_GLOBAL_PATH=$SEGAR_PATH
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

source "$__SEGAR_SDK_PATH/segar_cli_auto_complete.bash"
