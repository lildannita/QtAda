#!/bin/bash

parse_system_release()
{
    FPATH="$1"
    FULL_DISTRIBUTIVE_NAME=`head -1 ${FPATH}`
    if [ -z "${DISTRIBUTIVE_NAME}" ]; then
        DISTRIBUTIVE_NAME=`echo ${FULL_DISTRIBUTIVE_NAME} | awk '{ print $1 }'`
    fi
    if [ -z "${OS_RELEASE}" ]; then
        if [ "${DISTRIBUTIVE_NAME}" = "Oracle" -o "${DISTRIBUTIVE_NAME}" = "CentOS" ]; then
            OS_RELEASE=`echo ${FULL_DISTRIBUTIVE_NAME} | awk -F"release " '{ print $2 }' | awk -F"." '{ print $1 }'`
        fi
    fi
}

parse_os_release()
{
    PARSE_VAL="PRETTY_NAME"
    PARSE_DATA=`cat /etc/os-release | grep $PARSE_VAL | sed "s/^[ ]*$PARSE_VAL[ \t]*=[ \t]*/$PARSE_VAL=/g" | grep ^$PARSE_VAL\= | head -1 | sed -e "s/\"//g"`
    FULL_DISTRIBUTIVE_NAME=${PARSE_DATA#$PARSE_VAL=}
    DISTRIBUTIVE_NAME=`echo ${FULL_DISTRIBUTIVE_NAME} | awk '{ print $1 }'`
    PARSE_VAL="VERSION_ID"
    PARSE_DATA=`cat /etc/os-release | grep $PARSE_VAL | sed "s/^[ ]*$PARSE_VAL[ \t]*=[ \t]*/$PARSE_VAL=/g" | grep ^$PARSE_VAL\= | head -1 | sed -e "s/\"//g"`
    OS_RELEASE=${PARSE_DATA#$PARSE_VAL=}
}

OS_BIT_TYPE=`/bin/uname -m`
OS_RELEASE=""
FULL_DISTRIBUTIVE_NAME=""
DISTRIBUTIVE_NAME=""

if [ -f "/etc/os-release" ]; then
    parse_os_release
fi
if [ -f "/etc/system-release" ]; then
    parse_system_release "/etc/system-release"
elif [ -f "/etc/redhat-release" ]; then
    parse_system_release "/etc/redhat-release"
elif [ -f "/etc/astra_version" ]; then
    if [ -z "${FULL_DISTRIBUTIVE_NAME}" ]; then
        FULL_DISTRIBUTIVE_NAME=`head -1 /etc/astra_version`
    fi
    if [ -z "${DISTRIBUTIVE_NAME}" ]; then
        DISTRIBUTIVE_NAME="Astra"
    fi
    if [ -z "${OS_RELEASE}" ]; then
        OS_RELEASE=`echo ${FULL_DISTRIBUTIVE_NAME} | awk '{ print $2 }'`
    fi
elif [ -f "/etc/debian_version" ]; then
    if [ -z "${FULL_DISTRIBUTIVE_NAME}" ]; then
        FULL_DISTRIBUTIVE_NAME="Debian"
    fi
    if [ -z "${DISTRIBUTIVE_NAME}" ]; then
        DISTRIBUTIVE_NAME="Debian"
    fi
    if [ -z "${OS_RELEASE}" ]; then
        OS_RELEASE=`head -1 /etc/debian_version`
    fi
fi

if [ -f "/etc/issue" ]; then
    if [ "${FULL_DISTRIBUTIVE_NAME}" == "" -o "${DISTRIBUTIVE_NAME}" == "" ]; then
        FULL_DISTRIBUTIVE_NAME=`head -1 /etc/issue | sed -e 's/\\n//;s/\\l//;s/[ ]*$//'`
        if [ "${DISTRIBUTIVE_NAME}" == "" ]; then
            DISTRIBUTIVE_NAME=`echo ${FULL_DISTRIBUTIVE_NAME} | awk '{ print $1 }'`
        fi
    fi
    if [ "${OS_RELEASE}" == "" ]; then
        if [ "$DISTRIBUTIVE_NAME" = "Astra" ]; then
            OS_RELEASE=`head -1 /etc/issue | awk '{ print $4 }'`
        elif [ "$DISTRIBUTIVE_NAME" = "Debian" ]; then
            OS_RELEASE=`head -1 /etc/issue | awk '{ print $3 }'`
        else
            OS_RELEASE=`head -1 /etc/issue | awk '{ print $3 }' | cut -c 1`
        fi
    fi
fi

if [ "$DISTRIBUTIVE_NAME" = "RED" ]; then
    DISTRIBUTIVE_NAME="RedOS"
fi

FULL_OS_NAME="${DISTRIBUTIVE_NAME}${OS_RELEASE}"

msg=""
if [ "$1" = "-f" ]; then
    msg="${FULL_DISTRIBUTIVE_NAME} ${OS_BIT_TYPE}"
elif [ "$1" = "-d" ]; then
    msg="${DISTRIBUTIVE_NAME}"
else
    msg="${DISTRIBUTIVE_NAME} ${OS_RELEASE} ${OS_BIT_TYPE}"
fi

if [ -z "${DISTRIBUTIVE_NAME}" ]; then
    echo "$msg | Failed to determine the OS distribution name"
    exit 1
fi
if [ -z "${OS_RELEASE}" ]; then
    echo "$msg | Failed to determine the OS distribution release"
    exit 1
fi
if [ -z "${OS_BIT_TYPE}" ]; then
    echo "$msg | Failed to determine the OS distribution bitness"
    exit 1
fi

echo $msg
exit 0
