#!/bin/sh

# Copyright 2016, INRIA

# This script is supposed to be called from RIOTs make system,
# as it depends on certain environment variables.

# @author       Oliver Hahm <oliver.hahm@inria.fr>
# @author       Hauke Peteresen <hauke.petersen@fu-berlin.de>
# @author       Joakim Nohlgård <joakim.nohlgard@eistec.se>

ELFFILE=${1}
ARGS=${2}

VALGRIND=$(which valgrind)

if [ ! -f "${ELFFILE}" ]; then
    echo "Error: Unable to locate ELFFILE"
    echo "       (${ELFFILE})"
    exit 1
fi

# setsid is needed so that Ctrl+C in GDB doesn't kill valgrind
[ -z "${SETSID}" ] && SETSID="$(which setsid)"
# temporary file that saves valgrind pid
VALGRIND_PIDFILE=$(mktemp -t "valgrind_pid.XXXXXXXXXX")
# cleanup after script terminates
trap "cleanup ${VALGRIND_PIDFILE}" EXIT
# don't trap on Ctrl+C, because GDB keeps running
trap '' INT
# start Valgrind as GDB server
${SEDSID} sh -c "${VALGRIND} --vgdb=yes --vgdb-error=0 -v --leak-check=full \
    --track-origins=yes --fullpath-after=${RIOTBASE} --read-var-info=yes \
    ${ELFFILE} ${ARGS} &
    echo \$! > ${VALGRIND_PIDFILE}" &
    sleep 1
# connect to the GDB server
${DEBUGGER} -ex "target remote | vgdb --pid=$(cat ${VALGRIND_PIDFILE})" --args ${ELFFILE} ${ARGS}
# will be called by trap
cleanup() {
    VALGRIND_PID="$(cat ${VALGRIND_PIDFILE})"
    kill -9 ${VALGRIND_PID}
    rm -f "${VALGRIND_PIDFILE}"
    exit 0
}
