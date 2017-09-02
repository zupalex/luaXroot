SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

export LUA_PATH="${DIR}/?.lua;${DIR}/?;${DIR}/lua_modules/?.lua;${DIR}/lua_modules/?;${DIR}/../user/?.lua;${DIR}/../user/?"

LUAXROOTLIBPATH=$( cd $DIR/../exec/ > /dev/null; pwd)
export LUAXROOTLIBPATH

alias luaXroot="rlwrap ${LUAXROOTLIBPATH}/lua -i -e \"_G.LUAXROOTLIBPATH = \\\"${LUAXROOTLIBPATH}\\\"\" -l luaXrootlogon"
