/*
 * $Id$
 *
 * Copyright (C) 2013 OpenSIPS Solutions
 *
 * This file is part of opensips, a free SIP server.
 *
 * opensips is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * opensips is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * -------
 * 2013-02-28: Created (Liviu)
 */

#ifndef _REST_METHODS_
#define _REST_METHODS_

#include "../../pvar.h"
#include "../../dprint.h"
#include "../../error.h"
#include "../../mem/mem.h"

extern long connection_timeout;
extern long curl_timeout;

extern char *ssl_ca_path;
extern int ssl_verifypeer;
extern int ssl_verifyhost;

int rest_get_method(struct sip_msg *msg, char *url,
                    pv_spec_p body_pv, pv_spec_p ctype_pv, pv_spec_p code_pv);
int rest_post_method(struct sip_msg *msg, char *url, char *ctype, char *body,
                     pv_spec_p body_pv, pv_spec_p ctype_pv, pv_spec_p code_pv);

#endif /* _REST_METHODS_ */

