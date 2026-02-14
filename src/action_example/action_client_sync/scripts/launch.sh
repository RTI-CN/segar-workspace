# Resolve the project root from the script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJ_DIR="$SCRIPT_DIR/../../../"

LD_LIBRARY_PATH=$PROJ_DIR/third_party/lib:$PROJ_DIR/lib:$LD_LIBRARY_PATH
PATH=$SCRIPT_DIR/../bin:$PATH
SEGAR_PATH=$SCRIPT_DIR/..
export LD_LIBRARY_PATH PATH SEGAR_PATH
# create log dir
SEGAR_LOG_DIR_PREFIX="$PROJ_DIR/.segar/log"
echo $SEGAR_LOG_DIR_PREFIX
if [ ! -d "$SEGAR_LOG_DIR_PREFIX" ]; then
    mkdir -p "$SEGAR_LOG_DIR_PREFIX"
fi

export GLOG_log_dir="$SEGAR_LOG_DIR_PREFIX"
export GLOG_alsologtostderr=1
export GLOG_colorlogtostderr=1
export GLOG_minloglevel=0
export sysmo_start=0
export SEGAR_DOMAIN_ID=0
export SEGAR_IP=127.0.0.1
#gdb --args bin/action_client_sync
action_client_sync