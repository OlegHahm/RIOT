/*
 * Copyright (C) 2016 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 */

#include <stdio.h>

#include "idmanager.h"

int _gnrc_tisch_root(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    if (idmanager_getIsDAGroot()) {
        puts("gnrc_tisch: unset root status of node");
        idmanager_setIsDAGroot(false);
    }
    else {
        puts("gnrc_tisch: set root status of node");
        idmanager_setIsDAGroot(true);
    }

    return 0;
}

/** @} */

