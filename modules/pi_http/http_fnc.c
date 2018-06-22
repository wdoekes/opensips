/*
 * $Id$
 *
 * Copyright (C) 2011-2013 VoIP Embedded Inc.
 *
 * This file is part of Open SIP Server (opensips).
 *
 * opensips is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * opensips is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * History:
 * ---------
 *  2011-09-20  first version (osas)
 */

#include <libxml/parser.h>

#include "../../str.h"
#include "../../ut.h"
#include "../../db/db_ut.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "../../config.h"
#include "../../globals.h"
#include "../../socket_info.h"
#include "../../resolve.h"
#include "../../parser/parse_uri.h"

#include "../httpd/httpd_load.h"
#include "http_fnc.h"
#include "http_db_handler.h"


#define PI_HTTP_XML_FRAMEWORK_NODE	"framework"
#define PI_HTTP_XML_DB_URL_NODE		"db_url"
#define PI_HTTP_XML_DB_TABLE_NODE	"db_table"
#define PI_HTTP_XML_TABLE_NAME_NODE	"table_name"
#define PI_HTTP_XML_DB_URL_ID_NODE	"db_url_id"

#define PI_HTTP_XML_MOD_NODE		"mod"
#define PI_HTTP_XML_MOD_NAME_NODE	"mod_name"
#define PI_HTTP_XML_CMD_NODE		"cmd"
#define PI_HTTP_XML_DB_TABLE_ID_NODE	"db_table_id"
#define PI_HTTP_XML_CMD_NAME_NODE	"cmd_name"
#define PI_HTTP_XML_CMD_TYPE_NODE	"cmd_type"
#define PI_HTTP_XML_CLAUSE_COLS_NODE	"clause_cols"
#define PI_HTTP_XML_QUERY_COLS_NODE	"query_cols"
#define PI_HTTP_XML_ORDER_BY_COLS_NODE	"order_by_cols"

#define PI_HTTP_XML_COLUMN_NODE		"column"
#define PI_HTTP_XML_COL_NODE		"col"

#define PI_HTTP_XML_FIELD_NODE		"field"
#define PI_HTTP_XML_LINK_CMD_NODE	"link_cmd"
#define PI_HTTP_XML_TYPE_NODE		"type"
#define PI_HTTP_XML_OPERATOR_NODE	"operator"
#define PI_HTTP_XML_VALUE_NODE		"value"
#define PI_HTTP_XML_VALIDATE_NODE	"validate"

#define PI_HTTP_XML_ID_ATTR		"id"

extern str http_root;
extern int http_method;
extern httpd_api_t httpd_api;

ph_framework_t *ph_framework_data = NULL;

ph_html_page_data_t html_page_data;

#define PI_HTTP_DB_UNDEF	0
#define PI_HTTP_DB_QUERY	1
#define PI_HTTP_DB_INSERT	2
#define PI_HTTP_DB_DELETE	3
#define PI_HTTP_DB_UPDATE	4
#define PI_HTTP_DB_REPLACE	5


#define PI_HTTP_COPY(p,str)	\
do{	\
	if ((int)((p)-buf)+(str).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (str).s, (str).len); (p) += (str).len;	\
}while(0)

#define PI_HTTP_COPY_2(p,str1,str2)	\
do{	\
	if ((int)((p)-buf)+(str1).len+(str2).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (str1).s, (str1).len); (p) += (str1).len;	\
	memcpy((p), (str2).s, (str2).len); (p) += (str2).len;	\
}while(0)

#define PI_HTTP_COPY_3(p,str1,str2,str3)	\
do{	\
	if ((int)((p)-buf)+(str1).len+(str2).len+(str3).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (str1).s, (str1).len); (p) += (str1).len;	\
	memcpy((p), (str2).s, (str2).len); (p) += (str2).len;	\
	memcpy((p), (str3).s, (str3).len); (p) += (str3).len;	\
}while(0)

#define PI_HTTP_COPY_4(p,str1,str2,str3,str4)	\
do{	\
	if ((int)((p)-buf)+(str1).len+(str2).len+(str3).len+(str4).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (str1).s, (str1).len); (p) += (str1).len;	\
	memcpy((p), (str2).s, (str2).len); (p) += (str2).len;	\
	memcpy((p), (str3).s, (str3).len); (p) += (str3).len;	\
	memcpy((p), (str4).s, (str4).len); (p) += (str4).len;	\
}while(0)

#define PI_HTTP_COPY_5(p,s1,s2,s3,s4,s5)	\
do{	\
	if ((int)((p)-buf)+(s1).len+(s2).len+(s3).len+(s4).len+(s5).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (s1).s, (s1).len); (p) += (s1).len;	\
	memcpy((p), (s2).s, (s2).len); (p) += (s2).len;	\
	memcpy((p), (s3).s, (s3).len); (p) += (s3).len;	\
	memcpy((p), (s4).s, (s4).len); (p) += (s4).len;	\
	memcpy((p), (s5).s, (s5).len); (p) += (s5).len;	\
}while(0)

#define PI_HTTP_COPY_6(p,s1,s2,s3,s4,s5,s6)	\
do{	\
	if ((int)((p)-buf)+(s1).len+(s2).len+(s3).len+(s4).len+(s5).len+(s6).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (s1).s, (s1).len); (p) += (s1).len;	\
	memcpy((p), (s2).s, (s2).len); (p) += (s2).len;	\
	memcpy((p), (s3).s, (s3).len); (p) += (s3).len;	\
	memcpy((p), (s4).s, (s4).len); (p) += (s4).len;	\
	memcpy((p), (s5).s, (s5).len); (p) += (s5).len;	\
	memcpy((p), (s6).s, (s6).len); (p) += (s6).len;	\
}while(0)

#define PI_HTTP_COPY_8(p,s1,s2,s3,s4,s5,s6,s7,s8)	\
do{	\
	if ((int)((p)-buf)+(s1).len+(s2).len+(s3).len+(s4).len+(s5).len+(s6).len+(s7).len+(s8).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (s1).s, (s1).len); (p) += (s1).len;	\
	memcpy((p), (s2).s, (s2).len); (p) += (s2).len;	\
	memcpy((p), (s3).s, (s3).len); (p) += (s3).len;	\
	memcpy((p), (s4).s, (s4).len); (p) += (s4).len;	\
	memcpy((p), (s5).s, (s5).len); (p) += (s5).len;	\
	memcpy((p), (s6).s, (s6).len); (p) += (s6).len;	\
	memcpy((p), (s7).s, (s7).len); (p) += (s7).len;	\
	memcpy((p), (s8).s, (s8).len); (p) += (s8).len;	\
}while(0)

#define PI_HTTP_COPY_9(p,s1,s2,s3,s4,s5,s6,s7,s8,s9)	\
do{	\
	if ((int)((p)-buf)+(s1).len+(s2).len+(s3).len+(s4).len+(s5).len+(s6).len+(s7).len+(s8).len+(s9).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (s1).s, (s1).len); (p) += (s1).len;	\
	memcpy((p), (s2).s, (s2).len); (p) += (s2).len;	\
	memcpy((p), (s3).s, (s3).len); (p) += (s3).len;	\
	memcpy((p), (s4).s, (s4).len); (p) += (s4).len;	\
	memcpy((p), (s5).s, (s5).len); (p) += (s5).len;	\
	memcpy((p), (s6).s, (s6).len); (p) += (s6).len;	\
	memcpy((p), (s7).s, (s7).len); (p) += (s7).len;	\
	memcpy((p), (s8).s, (s8).len); (p) += (s8).len;	\
	memcpy((p), (s9).s, (s9).len); (p) += (s9).len;	\
}while(0)

#define PI_HTTP_COPY_10(p,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10)	\
do{	\
	if ((int)((p)-buf)+(s1).len+(s2).len+(s3).len+(s4).len+(s5).len+(s6).len+(s7).len+(s8).len+(s9).len+(s10).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (s1).s, (s1).len); (p) += (s1).len;	\
	memcpy((p), (s2).s, (s2).len); (p) += (s2).len;	\
	memcpy((p), (s3).s, (s3).len); (p) += (s3).len;	\
	memcpy((p), (s4).s, (s4).len); (p) += (s4).len;	\
	memcpy((p), (s5).s, (s5).len); (p) += (s5).len;	\
	memcpy((p), (s6).s, (s6).len); (p) += (s6).len;	\
	memcpy((p), (s7).s, (s7).len); (p) += (s7).len;	\
	memcpy((p), (s8).s, (s8).len); (p) += (s8).len;	\
	memcpy((p), (s9).s, (s9).len); (p) += (s9).len;	\
	memcpy((p), (s10).s, (s10).len); (p) += (s10).len;	\
}while(0)

#define PI_HTTP_COPY_11(p,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11)	\
do{	\
	if ((int)((p)-buf)+(s1).len+(s2).len+(s3).len+(s4).len+(s5).len+(s6).len+(s7).len+(s8).len+(s9).len+(s10).len+(s11).len>max_page_len) {	\
		goto error;	\
	}	\
	memcpy((p), (s1).s, (s1).len); (p) += (s1).len;	\
	memcpy((p), (s2).s, (s2).len); (p) += (s2).len;	\
	memcpy((p), (s3).s, (s3).len); (p) += (s3).len;	\
	memcpy((p), (s4).s, (s4).len); (p) += (s4).len;	\
	memcpy((p), (s5).s, (s5).len); (p) += (s5).len;	\
	memcpy((p), (s6).s, (s6).len); (p) += (s6).len;	\
	memcpy((p), (s7).s, (s7).len); (p) += (s7).len;	\
	memcpy((p), (s8).s, (s8).len); (p) += (s8).len;	\
	memcpy((p), (s9).s, (s9).len); (p) += (s9).len;	\
	memcpy((p), (s10).s, (s10).len); (p) += (s10).len;	\
	memcpy((p), (s11).s, (s11).len); (p) += (s11).len;	\
}while(0)


#define PI_HTTP_COMPLETE_REPLY(page,buffer,mod,cmd,fmt,args...)	\
do{								\
	_len = snprintf((page)->s + (page)->len,		\
			(buffer)->len - (page)->len,		\
			fmt, ##args);				\
	if(_len<0)						\
		goto error;					\
	else							\
		(page)->len += _len;				\
	p = page->s + page->len;				\
	PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_td_4d);	\
	page->len = p - page->s;				\
	if(ph_build_reply_footer((page), (buffer)->len)<0)	\
		goto error;					\
}while(0)


#define PI_HTTP_BUILD_REPLY(page,buffer,mod,cmd,fmt,args...)	\
do{								\
	if(ph_build_reply((page),(buffer)->len,(mod),(cmd))<0)	\
		goto error;					\
	_len = snprintf((page)->s + (page)->len,		\
			(buffer)->len - (page)->len,		\
			fmt, ##args);				\
	if(_len<0)						\
		goto error;					\
	else							\
		(page)->len += _len;				\
	p = page->s + page->len;				\
	PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_td_4d);	\
	page->len = p - page->s;				\
	if(ph_build_reply_footer((page), (buffer)->len)<0)	\
		goto error;					\
}while(0)

/* */
#define PI_HTTP_ESC_COPY(p,str,temp_holder,temp_counter)	\
do{	\
	(temp_holder).s = (str).s;	\
	(temp_holder).len = 0;	\
	for((temp_counter)=0;(temp_counter)<(str).len;(temp_counter)++) {	\
		switch((str).s[(temp_counter)]) {	\
		case '<':	\
			(temp_holder).len = (temp_counter) - (temp_holder).len;	\
			PI_HTTP_COPY_2(p, (temp_holder), PI_HTTP_ESC_LT);	\
			(temp_holder).s = (str).s + (temp_counter) + 1;	\
			(temp_holder).len = (temp_counter) + 1;	\
			break;	\
		case '>':	\
			(temp_holder).len = (temp_counter) - (temp_holder).len;	\
			PI_HTTP_COPY_2(p, (temp_holder), PI_HTTP_ESC_GT);	\
			(temp_holder).s = (str).s + (temp_counter) + 1;	\
			(temp_holder).len = (temp_counter) + 1;	\
			break;	\
		case '&':	\
			(temp_holder).len = (temp_counter) - (temp_holder).len;	\
			PI_HTTP_COPY_2(p, (temp_holder), PI_HTTP_ESC_AMP);	\
			(temp_holder).s = (str).s + (temp_counter) + 1;	\
			(temp_holder).len = (temp_counter) + 1;	\
			break;	\
		case '"':	\
			(temp_holder).len = (temp_counter) - (temp_holder).len;	\
			PI_HTTP_COPY_2(p, (temp_holder), PI_HTTP_ESC_QUOT);	\
			(temp_holder).s = (str).s + (temp_counter) + 1;	\
			(temp_holder).len = (temp_counter) + 1;	\
			break;	\
		case '\'':	\
			(temp_holder).len = (temp_counter) - (temp_holder).len;	\
			PI_HTTP_COPY_2(p, (temp_holder), PI_HTTP_ESC_SQUOT);	\
			(temp_holder).s = (str).s + (temp_counter) + 1;	\
			(temp_holder).len = (temp_counter) + 1;	\
			break;	\
		}	\
	}	\
	(temp_holder).len = (temp_counter) - (temp_holder).len;	\
	PI_HTTP_COPY(p, (temp_holder));	\
}while(0)


static const str PI_HTTP_METHOD[] = {
	str_init("GET"),
	str_init("POST")
};

static const str PI_HTTP_Response_Head_1 = str_init("<html><head><title>"\
	"OpenSIPS Provisionning Interface</title>"\
	"<style type=\"text/css\">"\
		"body{margin:0;}body,p,div,td,th,tr,form,ol,ul,li,input,textarea,select,"\
		"a{font-family:\"lucida grande\",verdana,geneva,arial,helvetica,sans-serif;font-size:14px;}"\
		"a:hover{text-decoration:none;}a{text-decoration:underline;}"\
		".foot{padding-top:40px;font-size:10px;color:#333333;}"\
		".foot a{font-size:10px;color:#000000;}"
		"table.center{margin-left:auto;margin-right:auto;}"\
	"</style>"\
	"<meta http-equiv=\"Expires\" content=\"0\">"\
	"<meta http-equiv=\"Pragma\" content=\"no-cache\">");


static const str PI_HTTP_Response_Head_2 = str_init(\
"<link rel=\"icon\" type=\"image/png\" href=\"http://opensips.org/favicon.png\">"\
"</head>\n"\
"<body alink=\"#000000\" bgcolor=\"#ffffff\" link=\"#000000\" text=\"#000000\" vlink=\"#000000\">");

static const str PI_HTTP_Response_Title_Table_1 = str_init(\
"<table cellspacing=\"0\" cellpadding=\"5\" width=\"100%%\" border=\"0\">"\
	"<tr bgcolor=\"#BBDDFF\">"\
	"<td colspan=2 valign=\"top\" align=\"left\" bgcolor=\"#EFF7FF\" width=\"100%%\">"\
	"<br/><h2 align=\"center\">OpenSIPS Provisionning Interface</h2>");
static const str PI_HTTP_Response_Title_Table_3 = str_init("<br/></td></tr></table>\n<center>\n");

static const str PI_HTTP_Response_Menu_Table_1 = str_init("<table border=\"0\" cellpadding=\"3\" cellspacing=\"0\"><tbody><tr>\n");
static const str PI_HTTP_Response_Menu_Table_2 = str_init("<td><a href='");
static const str PI_HTTP_Response_Menu_Table_2b = str_init("<td><b><a href='");
static const str PI_HTTP_Response_Menu_Table_3 = str_init("'>");
static const str PI_HTTP_Response_Menu_Table_4 = str_init("</a><td>\n");
static const str PI_HTTP_Response_Menu_Table_4b = str_init("</a></b><td>\n");
static const str PI_HTTP_Response_Menu_Table_5 = str_init("</tr></tbody></table>\n");

static const str PI_HTTP_Response_Menu_Cmd_Table_1a = str_init("<table border=\"0\" cellpadding=\"3\" cellspacing=\"0\" width=\"90%\"><tbody>\n");
static const str PI_HTTP_Response_Menu_Cmd_Table_1b = str_init("<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"90%\"><tbody>\n");
static const str PI_HTTP_Response_Menu_Cmd_tr_1 = str_init("<tr>\n");
static const str PI_HTTP_Response_Menu_Cmd_td_1a = str_init("	<td width=\"10%\"><a href='");
static const str PI_HTTP_Response_Menu_Cmd_td_4a = str_init("</a></td>\n");
static const str PI_HTTP_Response_Menu_Cmd_td_1b = str_init("	<td align=\"left\"><b>");
static const str PI_HTTP_Response_Menu_Cmd_td_1c = str_init("	<td valign=\"top\" align=\"left\" rowspan=\"");
static const str PI_HTTP_Response_Menu_Cmd_td_1d = str_init("	<td>");
static const str PI_HTTP_Response_Menu_Cmd_td_1e = str_init("	<td colspan=\"99\">");
static const str PI_HTTP_Response_Menu_Cmd_td_1f = str_init("	<td rowspan=\"999999\">");
static const str PI_HTTP_Response_Menu_Cmd_td_3c = str_init("\">");
static const str PI_HTTP_Response_Menu_Cmd_td_4b = str_init("</b></td>\n");
static const str PI_HTTP_Response_Menu_Cmd_td_4c = str_init("	</td>\n");
static const str PI_HTTP_Response_Menu_Cmd_td_4d = str_init("</td>\n");
static const str PI_HTTP_Response_Menu_Cmd_tr_2 = str_init("</tr>\n");
static const str PI_HTTP_Response_Menu_Cmd_Table_2 = str_init("</tbody></table>\n");

static const str PI_HTTP_NBSP = str_init("&nbsp;");
static const str PI_HTTP_SLASH = str_init("/");
static const str PI_HTTP_SQUOT_GT = str_init("'>");

static const str PI_HTTP_ATTR_SEPARATOR = str_init(" ");
static const str PI_HTTP_ATTR_VAL_SEPARATOR = str_init("=");

static const str PI_HTTP_Post_Form_1a = str_init("\n"\
"		<form name=\"input\" method=\"");
static const str PI_HTTP_Post_Form_1b = str_init("\">\n"
"			<input type=hidden name=cmd value=\"on\">\n");

static const str PI_HTTP_Post_Input = str_init(\
"			");
static const str PI_HTTP_Post_Clause_Input = str_init("<br/>Clause:");
static const str PI_HTTP_Post_Values_Input = str_init("<br/>Values:");
static const str PI_HTTP_Post_Query_Input = str_init("<table border=\"0\" cellpadding=\"3\" cellspacing=\"0\"><tbody>\n");
static const str PI_HTTP_Post_Input_1 = str_init(\
"			<tr><td><b>");
static const str PI_HTTP_Post_Input_Text = str_init(\
"</b></td><td><input type=\"text\" name=\"");
static const str PI_HTTP_Post_Input_Select_1 = str_init(\
"</b></td><td><select name=\"");
static const str PI_HTTP_Post_Input_Select_2 = str_init("\"/>");
static const str PI_HTTP_Post_Input_Option_1 = str_init(\
"\n				<option value=\"");
static const str PI_HTTP_Post_Input_Option_2 = str_init("\">");
static const str PI_HTTP_Post_Input_Option_3 = str_init("</option>");
static const str PI_HTTP_Post_Input_Select_3 = str_init("</td></tr>\n");
static const str PI_HTTP_Post_Input_Hidden_1 = str_init(\
"			<input type=hidden name=\"");
static const str PI_HTTP_Post_Input_Hidden_2 = str_init("\" value=\"");
static const str PI_HTTP_Post_Input_Hidden_3 = str_init("\">\n");
static const str PI_HTTP_Post_Input_3 = str_init("\"/></td></tr>\n");
static const str PI_HTTP_Post_Input_4 = str_init(\
"			</tbody></table>\n");
static const str PI_HTTP_Post_Form_2 = str_init(\
"			<br/><input type=\"submit\" value=\"Submit\"/>\n"\
"		</form>\n");

static const str PI_HTTP_Response_Foot = str_init(\
"\n</center>\n<div align=\"center\" class=\"foot\" style=\"margin:20px auto\">"\
	"<span style='margin-left:5px;'></span>"\
	"<a href=\"http://opensips.org\">OpenSIPS web site</a><br/>"\
	"Copyright &copy; 2012-2014 <a href=\"http://www.voipembedded.com/\">VoIP Embedded, Inc.</a>"\
								". All rights reserved."\
"</div></body></html>");

#define PI_HTTP_ROWSPAN 20
static const str PI_HTTP_CMD_ROWSPAN = str_init("20");

static const str PI_HTTP_ESC_LT =    str_init("&lt;");   /* < */
static const str PI_HTTP_ESC_GT =    str_init("&gt;");   /* > */
static const str PI_HTTP_ESC_AMP =   str_init("&amp;");  /* & */
static const str PI_HTTP_ESC_QUOT =  str_init("&quot;"); /* " */
static const str PI_HTTP_ESC_SQUOT = str_init("&#39;");  /* ' */

static const str PI_HTTP_HREF_1 = str_init("<a href='/");
static const str PI_HTTP_HREF_2 = str_init("?cmd=pre&");
static const str PI_HTTP_HREF_3 = str_init("</a>");


xmlAttrPtr ph_xmlNodeGetAttrByName(xmlNodePtr node, const char *name)
{
	xmlAttrPtr attr = node->properties;
	while (attr) {
		if(xmlStrcasecmp(attr->name, (const xmlChar*)name)==0)
			return attr;
		attr = attr->next;
	}
	return NULL;
}

char *ph_xmlNodeGetAttrContentByName(xmlNodePtr node, const char *name)
{
	xmlAttrPtr attr = ph_xmlNodeGetAttrByName(node, name);
	if (attr) return (char*)xmlNodeGetContent(attr->children);
	else return NULL;
}

xmlNodePtr ph_xmlNodeGetNodeByName(xmlNodePtr node, const char *name)
{
	xmlNodePtr cur = node;
	while (cur) {
		if(xmlStrcasecmp(cur->name, (const xmlChar*)name)==0)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

char *ph_xmlNodeGetNodeContentByName(xmlNodePtr node, const char *name)
{
	xmlNodePtr node1 = ph_xmlNodeGetNodeByName(node, name);
	if (node1) return (char*)xmlNodeGetContent(node1);
	else return NULL;
}


int ph_getDbUrlNodes(ph_framework_t *_ph_framework_data, xmlNodePtr framework_node)
{
	int i;
	xmlNodePtr node;
	ph_db_url_t *db_urls;
	ph_db_url_t *ph_db_urls = NULL;
	str id, db_url;

	//_ph_framework_data->ph_db_urls_size=0;
	for(node=framework_node->children;node;node=node->next){
		if (xmlStrcasecmp(node->name,
			(const xmlChar*)PI_HTTP_XML_DB_URL_NODE) == 0) {
			if(_ph_framework_data->ph_db_urls_size)
				db_urls = (ph_db_url_t*)
					shm_realloc(_ph_framework_data->ph_db_urls,
					(_ph_framework_data->ph_db_urls_size+1)*
					sizeof(ph_db_url_t));
			else
				db_urls = (ph_db_url_t*)
						shm_malloc(sizeof(ph_db_url_t));
			if(db_urls==NULL) {LM_ERR("oom\n");return -1;}
			_ph_framework_data->ph_db_urls = db_urls;

			ph_db_urls = _ph_framework_data->ph_db_urls;
			db_urls = &ph_db_urls[_ph_framework_data->ph_db_urls_size];
			memset(db_urls, 0, sizeof(ph_db_url_t));

			id.s = ph_xmlNodeGetAttrContentByName(node,
						PI_HTTP_XML_ID_ATTR);
			if(id.s==NULL){
				LM_ERR("No attribute for node %d [%s]\n",
					_ph_framework_data->ph_db_urls_size,
					node->name);
				return -1;
			}
			id.len = strlen(id.s);
			if(id.len==0){
				LM_ERR("Empty attr for node %d [%s]\n",
					_ph_framework_data->ph_db_urls_size,
					node->name);
				return -1;
			}
			if(shm_str_dup(&db_urls->id, &id)!=0) return -1;
			xmlFree(id.s);id.s=NULL;id.len=0;

			db_url.s = (char*)xmlNodeGetContent(node);
			if(db_url.s==NULL){
				LM_ERR("No content for node [%.*s][%s]\n",
					db_urls->id.len, db_urls->id.s, node->name);
				return -1;
			}
			db_url.len = strlen(db_url.s);
			if(db_url.len==0){
				LM_ERR("Empty content for node [%.*s][%s]\n",
					db_urls->id.len, db_urls->id.s, node->name);
				return -1;
			}
			if(shm_str_dup(&db_urls->db_url, &db_url)!=0) return -1;
			xmlFree(db_url.s);db_url.s=NULL;db_url.len=0;

			for(i=0;i<_ph_framework_data->ph_db_urls_size;i++){
				if(db_urls->id.len==ph_db_urls[i].id.len &&
                                        strncmp(ph_db_urls[i].id.s,
                                                db_urls->id.s,
						db_urls->id.len)==0){
					LM_ERR("duplicated %s %s [%.*s]\n",
						PI_HTTP_XML_DB_URL_NODE,
						PI_HTTP_XML_ID_ATTR,
						db_urls->id.len,
						db_urls->id.s);
					return -1;
				}
				if(db_urls->db_url.len==ph_db_urls[i].db_url.len &&
                                        strncmp(ph_db_urls[i].db_url.s,
                                                db_urls->db_url.s,
						db_urls->db_url.len)==0){
					LM_ERR("duplicated %s [%.*s]\n",
						PI_HTTP_XML_DB_URL_NODE,
						db_urls->db_url.len,
						db_urls->db_url.s);
					return -1;
				}
			}
			/* */
			LM_DBG("got node [%s]->[%.*s][%.*s]\n",
				node->name,
				db_urls->id.len, db_urls->id.s,
				db_urls->db_url.len, db_urls->db_url.s);
			/* */
			_ph_framework_data->ph_db_urls_size++;
		}
	}
	if(_ph_framework_data->ph_db_urls_size==0){
		LM_ERR("No [%s] node in config file\n", PI_HTTP_XML_DB_URL_NODE);
		return -1;
	}
	/* */
	for(i=0;i<_ph_framework_data->ph_db_urls_size;i++){
		LM_DBG("got node %s [%d][%.*s][%.*s]\n", PI_HTTP_XML_DB_URL_NODE,
			i, ph_db_urls[i].id.len, ph_db_urls[i].id.s,
			ph_db_urls[i].db_url.len, ph_db_urls[i].db_url.s);
	}
	/* */
	return 0;
}

int ph_getDbTableCols(ph_db_url_t *ph_db_urls, ph_db_table_t *db_tables,
			const xmlNodePtr table_node)
{
	int i;
	char *val;
	int val_len;
	xmlNodePtr node;
	ph_table_col_t *cols;
	str field;

	for(node=table_node->children,db_tables->cols_size=0;node;node=node->next){
		if (xmlStrcasecmp(node->name,
			(const xmlChar*)PI_HTTP_XML_COLUMN_NODE) == 0) {
			if(db_tables->cols_size)
				cols = (ph_table_col_t*)shm_realloc(db_tables->cols,
						(db_tables->cols_size+1)*
						sizeof(ph_table_col_t));
			else
				cols = (ph_table_col_t*)
					shm_malloc(sizeof(ph_table_col_t));
			if(cols==NULL) {LM_ERR("oom\n");return -1;}
			db_tables->cols = cols;
			cols = &db_tables->cols[db_tables->cols_size];
			memset(cols, 0, sizeof(ph_table_col_t));
			cols->type=-1;
			/* Populate the field */
			field.s =
				ph_xmlNodeGetNodeContentByName(node->children,
							PI_HTTP_XML_FIELD_NODE);
			if(field.s==NULL){
				LM_ERR("missing node %s [%.*s] %s %s\n",
					table_node->name,
					db_tables->name.len, db_tables->name.s,
					node->name, PI_HTTP_XML_FIELD_NODE);
				return -1;
			}
			field.len = strlen(field.s);
			if(field.len==0){
				LM_ERR("empty node %s [%.*s] %s %s \n",
					table_node->name,
					db_tables->name.len, db_tables->name.s,
					node->name, PI_HTTP_XML_FIELD_NODE);
				return -1;
			}
			if(shm_str_dup(&cols->field, &field)!=0) return -1;
			xmlFree(field.s);field.s=NULL;field.len=0;
			/* Each field MUST be unique */
			for(i=0;i<db_tables->cols_size;i++){
				if(cols->field.len==db_tables->cols[i].field.len &&
					strncmp(db_tables->cols[i].field.s,
						cols->field.s,
						cols->field.len)==0){
					LM_ERR("duplicated %s %s [%.*s] in %s [%.*s]\n",
						node->name, PI_HTTP_XML_FIELD_NODE,
						cols->field.len, cols->field.s,
						table_node->name,
						db_tables->name.len, db_tables->name.s);
					return -1;
				}
			}
			/* Populate the validation */
			val = ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_VALIDATE_NODE);
			if(val){
				val_len = strlen(val);
				switch(val_len){
				case 0:
					break;
				case 3:
					if(strncmp("URI",val,3)==0)
						cols->validation|=PH_FLAG_URI;
					break;
				case 11:
					if(strncmp("P_HOST_PORT",val,11)==0)
						cols->validation|=PH_FLAG_P_HOST_PORT;
					else
					if(strncmp("P_IPV4_PORT",val,11)==0)
						cols->validation|=PH_FLAG_P_IPV4_PORT;
					break;
				case 12:
					if(strncmp("URI_IPV4HOST",val,12)==0)
						cols->validation|=PH_FLAG_URI_IPV4HOST;
					break;
				default:
					LM_ERR("unexpected validation flag [%s] for "
						"%s %s %s\n",
						val, table_node->name, node->name,
						PI_HTTP_XML_VALIDATE_NODE);
					xmlFree(val);
					return -1;
				}
				xmlFree(val);
			}
			/* Populate the type */
			val = ph_xmlNodeGetNodeContentByName(node->children,
							PI_HTTP_XML_TYPE_NODE);
			if(val==NULL){
				LM_ERR("missing node %s %s %s\n",
					table_node->name, node->name,
					PI_HTTP_XML_TYPE_NODE);
				return -1;
			}
			val_len = strlen(val);
			if(val_len==0){
				LM_ERR("empty node %s %s %s\n",
					table_node->name, node->name,
					PI_HTTP_XML_TYPE_NODE);
				xmlFree(val);
				return -1;
			}else if(val_len==6){
				if(strncmp("DB_INT",val,6)==0)
					cols->type=DB_INT;
				else if(strncmp("DB_STR",val,6)==0)
					cols->type=DB_STR;
			}else if(val_len==7){
				if(strncmp("DB_BLOB",val,7)==0)
					cols->type=DB_BLOB;
			}else if(val_len==9){
				if(strncmp("DB_BIGINT",val,9)==0)
					cols->type=DB_BIGINT;
				else if(strncmp("DB_DOUBLE",val,9)==0)
					cols->type=DB_DOUBLE;
				else if(strncmp("DB_STRING",val,9)==0)
					cols->type=DB_STRING;
				else if(strncmp("DB_BITMAP",val,9)==0)
					cols->type=DB_BITMAP;
			}else if(val_len==11){
				if(strncmp("DB_DATETIME",val,11)==0)
					cols->type=DB_DATETIME;
			}
			if(cols->type==-1){
				LM_ERR("unexpected type [%s] for %s %s %s\n",
					val, table_node->name, node->name,
					PI_HTTP_XML_TYPE_NODE);
				xmlFree(val);
				return -1;
			}
			xmlFree(val);
			/* */
			LM_DBG("got node %s [%d][%.*s][%d]\n",
				PI_HTTP_XML_COLUMN_NODE,
				db_tables->cols_size,
				db_tables->cols[db_tables->cols_size].field.len,
				db_tables->cols[db_tables->cols_size].field.s,
				db_tables->cols[db_tables->cols_size].type);
			/* */
			db_tables->cols_size++;
		}
	}
	if(db_tables->cols_size==0){
		LM_ERR("missing node: %s %s\n",
			table_node->name, PI_HTTP_XML_COLUMN_NODE);
		return -1;
	}
	return 0;
}

int ph_getDbTables(ph_framework_t *_ph_framework_data, xmlNodePtr framework_node)
{
	int i;
	int j;
	char *val;
	int val_len;
	xmlNodePtr node;
	ph_db_table_t *db_tables;
	ph_db_table_t *ph_db_tables = NULL;
	ph_db_url_t *ph_db_urls;
	str id, name;

	//_ph_framework_data->ph_db_tables_size=0;
	for(node=framework_node->children;node;node=node->next){
		if (xmlStrcasecmp(node->name,
			(const xmlChar*)PI_HTTP_XML_DB_TABLE_NODE) == 0) {
			if(_ph_framework_data->ph_db_tables_size)
				db_tables = (ph_db_table_t*)
					shm_realloc(_ph_framework_data->ph_db_tables,
					(_ph_framework_data->ph_db_tables_size+1)*
					sizeof(ph_db_table_t));
			else
				db_tables = (ph_db_table_t*)
						shm_malloc(sizeof(ph_db_table_t));
			if(db_tables==NULL) {LM_ERR("oom\n"); return -1;}
			_ph_framework_data->ph_db_tables = db_tables;

			ph_db_tables = _ph_framework_data->ph_db_tables;
			db_tables = &ph_db_tables[_ph_framework_data->ph_db_tables_size];
			memset(db_tables, 0, sizeof(ph_db_table_t));

			/* Populate table ids */
			id.s = ph_xmlNodeGetAttrContentByName(node,
						PI_HTTP_XML_ID_ATTR);
			if(id.s==NULL){
				LM_ERR("No attribute for node %d [%s]\n",
					_ph_framework_data->ph_db_tables_size,
					node->name);
				return -1;
			}
			id.len = strlen(id.s);
			if(id.len==0){
				LM_ERR("Empty attr for node %d [%s]\n",
					_ph_framework_data->ph_db_tables_size,
					node->name);
				return -1;
			}
			if(shm_str_dup(&db_tables->id, &id)!=0) return -1;
			xmlFree(id.s);id.s=NULL;id.len=0;
			/* Populate table name */
			name.s =
				ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_TABLE_NAME_NODE);
			if(name.s==NULL){
				LM_ERR("No content for node [%.*s][%s]\n",
					db_tables->id.len, db_tables->id.s,
					node->name);
				return -1;
			}
			name.len = strlen(name.s);
			if(name.len==0){
				LM_ERR("Empty content for node [%.*s][%s]\n",
					db_tables->id.len, db_tables->id.s,
					node->name);
				return -1;
			}
			if(shm_str_dup(&db_tables->name, &name)!=0) return -1;
			xmlFree(name.s);name.s=NULL;name.len=0;
			/* Each table_id MUST be unique */
			for(i=0;i<_ph_framework_data->ph_db_tables_size;i++){
				if(db_tables->id.len==ph_db_tables[i].id.len &&
                                        strncmp(ph_db_tables[i].id.s,
                                                db_tables->id.s,
						db_tables->id.len)==0){
					LM_ERR("duplicated id %s %s [%.*s]\n",
						PI_HTTP_XML_DB_TABLE_NODE,
						PI_HTTP_XML_ID_ATTR,
						db_tables->id.len, db_tables->id.s);
					return -1;
				}
			}
			/* Populate the db_url_index */
			val = ph_xmlNodeGetNodeContentByName(node->children,
							PI_HTTP_XML_DB_URL_ID_NODE);
			if(val==NULL){
				LM_ERR("no %s for node %s [%.*s]\n",
					PI_HTTP_XML_DB_URL_ID_NODE,
					PI_HTTP_XML_DB_TABLE_NODE,
					db_tables->name.len, db_tables->name.s);
				return -1;
			}
			val_len = strlen(val);
			if(val_len==0){
				LM_ERR("empty %s for node %s [%.*s]\n",
					PI_HTTP_XML_DB_URL_ID_NODE,
					PI_HTTP_XML_DB_TABLE_NODE,
					db_tables->name.len, db_tables->name.s);
				return -1;
			}
			/* Get the db_url */
			for(i=0;i<_ph_framework_data->ph_db_urls_size;i++){
				ph_db_urls = _ph_framework_data->ph_db_urls;
				if(val_len==ph_db_urls[i].id.len &&
					strncmp(ph_db_urls[i].id.s, val, val_len)==0){
					db_tables->db_url = &ph_db_urls[i];
					break;
				}
			}
			if (i==_ph_framework_data->ph_db_urls_size){
				LM_ERR("bogus %s [%s] for node %s [%.*s]\n",
					PI_HTTP_XML_DB_URL_ID_NODE, val,
					PI_HTTP_XML_DB_TABLE_NODE,
					db_tables->name.len, db_tables->name.s);
				return -1;
			}
			xmlFree(val);

			if(ph_getDbTableCols(_ph_framework_data->ph_db_urls,
							db_tables, node)!=0)
				return -1;

			_ph_framework_data->ph_db_tables_size++;
			/*
			LM_DBG("got node [%s]->[%s][%s]\n",
				node->name, db_tables->id.s, db_tables->db_table.s);
			*/
		}
	}
	if(_ph_framework_data->ph_db_tables_size==0){
		LM_ERR("No [%s] node in config file\n", PI_HTTP_XML_DB_TABLE_NODE);
		return -1;
	}
	/* */
	for(i=0;i<_ph_framework_data->ph_db_tables_size;i++){
		LM_DBG("got node %s [%d][%.*s]->[%.*s/%.*s]\n",
			PI_HTTP_XML_DB_TABLE_NODE, i,
			ph_db_tables[i].id.len, ph_db_tables[i].id.s,
			ph_db_tables[i].db_url->db_url.len,
			ph_db_tables[i].db_url->db_url.s,
			ph_db_tables[i].name.len, ph_db_tables[i].name.s);
		db_tables = &ph_db_tables[i];
		for(j=0;j<db_tables->cols_size;j++){
			LM_DBG("got node %s [%.*s][%d]\n",
				PI_HTTP_XML_COLUMN_NODE,
				db_tables->cols[j].field.len,
				db_tables->cols[j].field.s,
				db_tables->cols[j].type);
		}
	}
	/* */
	return 0;
}

int ph_getColVals(ph_mod_t *module, ph_cmd_t *cmd,
		ph_vals_t *cmd_col_vals, xmlNodePtr col_node)
{
	xmlNodePtr node;
	str *vals, *col_vals = NULL;
	str *ids, *col_ids = NULL;
	int size = 0;
	int i;
	str attr;
	str val;

	for(node=col_node->children;node;node=node->next){
		if (xmlStrcasecmp(node->name,
			(const xmlChar*)PI_HTTP_XML_VALUE_NODE) == 0) {
			if(size){
				vals = (str*)shm_realloc(col_vals,
					(size+1)*sizeof(str));
				ids = (str*)shm_realloc(col_ids,
					(size+1)*sizeof(str));
			}else{
				vals = (str*)shm_malloc(sizeof(str));
				ids = (str*)shm_malloc(sizeof(str));
			}
			if(vals==NULL||ids==NULL) {LM_ERR("oom\n"); return -1;}
			col_vals = vals; col_ids = ids;
			vals = &col_vals[size]; ids = &col_ids[size];
			memset(vals, 0, sizeof *vals); memset(ids, 0, sizeof *ids);
			/* Retrieve the node attribute */
			attr.s = ph_xmlNodeGetAttrContentByName(node,
							PI_HTTP_XML_ID_ATTR);
			if(attr.s==NULL){
				LM_ERR("No attribute for node\n");
				return -1;
			}
			attr.len = strlen(attr.s);
			if(attr.len==0){
				LM_ERR("No attribute for node\n");
				return -1;
			}
			if(shm_str_dup(ids, &attr)!=0) return -1;
			xmlFree(attr.s); attr.s = NULL; attr.len = 0;
			/* Retrieve the node value */
			val.s = (char*)xmlNodeGetContent(node);
			if(val.s==NULL){
				LM_ERR("No content for node\n");
				return -1;
			}
			val.len = strlen(val.s);
			if(val.len==0){
				LM_ERR("No content for node\n");
				return -1;
			}
			if(shm_str_dup(vals, &val)!=0) return -1;
			xmlFree(val.s); val.s = NULL; val.len = 0;
			/*
			LM_DBG(">  > [%d] [%p]->[%s] [%p]->[%s]\n",
					size, ids, ids->s, vals, vals->s);
			*/
			size++;
		}
	}
	if(size){
		cmd_col_vals->ids = col_ids;
		cmd_col_vals->vals = col_vals;
		cmd_col_vals->vals_size = size;
		/* */
		for(i=0;i<size;i++)
			LM_DBG(">>> [%d] [%p]->[%.*s] [%p]->[%.*s]\n", i,
				cmd_col_vals->ids[i].s,
				cmd_col_vals->ids[i].len, cmd_col_vals->ids[i].s,
				cmd_col_vals->vals[i].s,
				cmd_col_vals->vals[i].len, cmd_col_vals->vals[i].s);
		/* */
	}
	return 0;
}

int ph_getCols(ph_mod_t *module, ph_cmd_t *cmd,
		db_op_t **mod_cmd_ops, db_key_t **mod_cmd_keys,
		db_type_t **mod_cmd_types, ph_vals_t **mod_cmd_vals, str **mod_cmd_linkCmd,
		int *key_size, xmlNodePtr cmd_node)
{
	xmlNodePtr node;
	str *key;
	str field;
	db_key_t *keys;
	db_key_t *cmd_keys = NULL;
	int op_len;
	char *operator;
	char *_operator;
	db_op_t *ops;
	db_op_t *cmd_ops = NULL;
	db_type_t *types;
	db_type_t *cmd_types = NULL;
	ph_vals_t *vals;
	ph_vals_t *cmd_vals = NULL;
	str link_cmd;
	str *linkCmd;
	str *cmd_linkCmd = NULL;
	int i;
	int size = 0;
	int table_size;
	ph_table_col_t *table_cols;

	for(node=cmd_node->children;node;node=node->next){
		if (xmlStrcasecmp(node->name,
			(const xmlChar*)PI_HTTP_XML_COL_NODE) == 0) {
			if(size)
				keys = (db_key_t*)shm_realloc(cmd_keys,
					(size+1)*sizeof(db_key_t));
			else
				keys = (db_key_t*)shm_malloc(sizeof(db_key_t));
			if (keys==NULL) {LM_ERR("oom\n");return -1;}
			cmd_keys = keys;
			keys = &cmd_keys[size];
			memset(keys, 0, sizeof(db_key_t));
			key = (str*)shm_malloc(sizeof(str));
			if (key==NULL) {LM_ERR("oom\n");return -1;}
			/* get the col field */
			field.s = ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_FIELD_NODE);
			if(field.s==NULL){
				LM_ERR("no %s in %s [%.*s] %s [%.*s] %s %s\n",
					PI_HTTP_XML_FIELD_NODE,
					cmd_node->parent->parent->name,
					module->module.len, module->module.s,
					cmd_node->parent->name,
					cmd->name.len, cmd->name.s,
					cmd_node->name, node->name);
				return -1;
			}
			field.len = strlen(field.s);
			if(field.len==0){
				LM_ERR("empty %s in %s [%.*s] %s [%.*s] %s %s\n",
					PI_HTTP_XML_FIELD_NODE,
					cmd_node->parent->parent->name,
					module->module.len, module->module.s,
					cmd_node->parent->name,
					cmd->name.len, cmd->name.s,
					cmd_node->name, node->name);
				return -1;
			}
			/* Each field must be valid */
			table_size = cmd->db_table->cols_size;
			table_cols = cmd->db_table->cols;
			for(i=0;i<table_size;i++){
				if(field.len==table_cols[i].field.len &&
					strncmp(table_cols[i].field.s,
						field.s,
						field.len)==0) break;
			}
			if(i==table_size){
				LM_ERR("invalid %s [%.*s] in %s [%.*s] %s [%.*s] %s %s"
					" for [%.*s]\n",
					PI_HTTP_XML_FIELD_NODE,
					field.len, field.s,
					cmd_node->parent->parent->name,
					module->module.len, module->module.s,
					cmd_node->parent->name,
					cmd->name.len, cmd->name.s,
					cmd_node->name, node->name,
					cmd->db_table->name.len, cmd->db_table->name.s);
				return -1;
			}
			if(shm_str_dup(key, &field)) return -1;
			*keys = key;
			xmlFree(field.s); field.s = NULL; field.len = 0;
			LM_DBG("cmd_keys=[%p] keys=[%p]->[%p]->[%.*s]\n",
						cmd_keys, keys, *keys,
						(*keys)->len, (*keys)->s);
			/* Retrieve the type */
			if(mod_cmd_types){
				if(size)
					types = (db_type_t*)shm_realloc(cmd_types,
						(size+1)*sizeof(db_type_t));
				else
					types =
					(db_type_t*)shm_malloc(sizeof(db_type_t));
				if (types==NULL) {LM_ERR("oom\n");return -1;}
				cmd_types = types;
				types = &cmd_types[size];
				memset(types, 0, sizeof(db_type_t));
				*types = table_cols[i].type;
			}
			/* Retrieve the ops */
			if(mod_cmd_ops){
				if(size)
					ops = (db_op_t*)shm_realloc(cmd_ops,
						(size+1)*sizeof(db_op_t));
				else
					ops = (db_op_t*)shm_malloc(sizeof(db_op_t));
				if (ops==NULL) {LM_ERR("oom\n");return -1;}
				cmd_ops = ops;
				ops = &cmd_ops[size];
				memset(ops, 0, sizeof(db_op_t));
				/* Retrieve the col op */
				operator =
					ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_OPERATOR_NODE);
				if(operator==NULL){
					LM_ERR("no %s in %s [%.*s] %s [%.*s] %s %s\n",
						PI_HTTP_XML_OPERATOR_NODE,
						cmd_node->parent->parent->name,
						module->module.len, module->module.s,
						cmd_node->parent->name,
						cmd->name.len, cmd->name.s,
						cmd_node->name, node->name);
					return -1;
				}
				op_len = strlen(operator);
				if(op_len==0){
					LM_ERR("empty %s in %s [%.*s] %s [%.*s] %s %s\n",
						PI_HTTP_XML_OPERATOR_NODE,
						cmd_node->parent->parent->name,
						module->module.len, module->module.s,
						cmd_node->parent->name,
						cmd->name.len, cmd->name.s,
						cmd_node->name, node->name);
					return -1;
				}else if(op_len==1){
					if(strncmp(OP_LT,operator,1)==0)
						*ops = operator;
					else if(strncmp(OP_GT,operator,1)==0)
						*ops = operator;
					else if(strncmp(OP_EQ,operator,1)==0)
						*ops = operator;
				}else if(op_len==2){
					if(strncmp(OP_LEQ,operator,2)==0)
						*ops = operator;
					else if(strncmp(OP_GEQ,operator,2)==0)
						*ops = operator;
					else if(strncmp(OP_NEQ,operator,2)==0)
						*ops = operator;
				}
				if(*ops==NULL){
					LM_ERR("unexpected %s [%s] in "
						"%s [%.*s] %s [%.*s] %s %s\n",
						PI_HTTP_XML_OPERATOR_NODE,
						operator,
						cmd_node->parent->parent->name,
						module->module.len, module->module.s,
						cmd_node->parent->name,
						cmd->name.len, cmd->name.s,
						cmd_node->name, node->name);
					return -1;
				}
				/* We need to copy the null string terminator */
				op_len++;
				_operator = shm_malloc(op_len);
				if(_operator==NULL){LM_ERR("oom\n"); return -1;}
				memcpy(_operator, operator, op_len);
				*ops = _operator;
				xmlFree(operator); operator = NULL; op_len = 0;
				LM_DBG("%s [%p]=>[%p]=>[%.*s] %s [%.*s] %s %s [%.*s][%s]\n",
					cmd_node->parent->parent->name, module,
					module->module.s,
					module->module.len, module->module.s,
					cmd_node->parent->name,
					cmd->name.len, cmd->name.s,
					cmd_node->name, node->name,
					(**keys).len, (**keys).s, *ops);
			}else{
				LM_DBG("%s [%p]=>[%p]=>[%.*s] %s [%.*s] %s %s [%.*s][]\n",
					cmd_node->parent->parent->name, module,
					module->module.s,
					module->module.len, module->module.s,
					cmd_node->parent->name,
					cmd->name.len, cmd->name.s,
					cmd_node->name, node->name,
					(**keys).len, (**keys).s);
			}
			/* Retrieve the vals */
			if(mod_cmd_vals){
				if(size)
					vals = (ph_vals_t*)shm_realloc(cmd_vals,
						(size+1)*sizeof(ph_vals_t));
				else
					vals =
					(ph_vals_t*)shm_malloc(sizeof(ph_vals_t));
				if(vals==NULL) {LM_ERR("oom\n");return -1;}
				cmd_vals = vals;
				vals = &cmd_vals[size];
				memset(vals, 0, sizeof(ph_vals_t));
				if(ph_getColVals(module, cmd, vals, node)!=0)
					return -1;
			}
			/* Retrieve the link_cmds */
			if(mod_cmd_linkCmd){
				if(size)
					linkCmd = (str*)shm_realloc(cmd_linkCmd,
						(size+1)*sizeof(str));
				else
					linkCmd = (str*)shm_malloc(sizeof(str));
				if(linkCmd==NULL) {LM_ERR("oom\n");return -1;}
				cmd_linkCmd = linkCmd;
				linkCmd = &cmd_linkCmd[size];
				memset(linkCmd, 0, sizeof(str));
				/* get the link_cmd */
				link_cmd.s = ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_LINK_CMD_NODE);
				if(link_cmd.s!=NULL){
					link_cmd.len = strlen(link_cmd.s);
					if(link_cmd.len!=0){
						LM_DBG("got %s=[%.*s] in %s [%.*s] %s [%.*s] %s %s\n",
							PI_HTTP_XML_LINK_CMD_NODE,
							link_cmd.len, link_cmd.s,
							cmd_node->parent->parent->name,
							module->module.len, module->module.s,
							cmd_node->parent->name,
							cmd->name.len, cmd->name.s,
							cmd_node->name, node->name);
						if(shm_str_dup(linkCmd, &link_cmd)) return -1;
					}
					xmlFree(link_cmd.s); link_cmd.s = NULL; link_cmd.len = 0;
				}
			}
			size++;
		}
	}
	if(size==0){
		LM_ERR("empty %s in %s [%.*s] %s [%.*s]\n",
			cmd_node->name, cmd_node->parent->parent->name,
			module->module.len, module->module.s,
			cmd_node->parent->name, cmd->name.len, cmd->name.s);
		return -1;
	}else if(cmd_keys!=NULL){
		*mod_cmd_keys = cmd_keys;
		LM_DBG("***  mod_cmd_keys=[%p] *mod_cmd_keys=[%p] cmd_keys=[%p]\n",
				mod_cmd_keys, *mod_cmd_keys, cmd_keys);
		if(cmd_keys) for(i=0;i<size;i++)
			LM_DBG("cmd_keys[%d]=[%p]->[%p]->[%.*s]\n",
				i, &cmd_keys[i], cmd_keys[i]->s,
				cmd_keys[i]->len, cmd_keys[i]->s);
		if(mod_cmd_ops) *mod_cmd_ops = cmd_ops;
		if(mod_cmd_types) *mod_cmd_types = cmd_types;
		if(mod_cmd_vals&&cmd_vals) *mod_cmd_vals = cmd_vals;
		if(mod_cmd_linkCmd) *mod_cmd_linkCmd = cmd_linkCmd;
		if(cmd_vals) for(i=0;i<size;i++){
			LM_DBG("cmd_vals[%d]=[%p]->[%d][%p][%p]\n",
				i, &cmd_vals[i], cmd_vals[i].vals_size,
				cmd_vals[i].ids, cmd_vals[i].vals);
			for(op_len=0;op_len<cmd_vals[i].vals_size;op_len++)
				LM_DBG("    [%d][%d] [%p]->[%.*s] [%p]->[%.*s]\n",
					i, op_len,
					&cmd_vals[i].ids[op_len],
					cmd_vals[i].ids[op_len].len,
					cmd_vals[i].ids[op_len].s,
					&cmd_vals[i].vals[op_len],
					cmd_vals[i].vals[op_len].len,
					cmd_vals[i].vals[op_len].s);
		}
		if(mod_cmd_linkCmd&&cmd_linkCmd) *mod_cmd_linkCmd = cmd_linkCmd;
		*key_size = size;
		if(cmd_ops) for(i=0;i<size;i++)
			LM_DBG("cmd_ops[%d]=[%p]->[%s]\n",
				i, &cmd_ops[i], cmd_ops[i]);
		LM_DBG("\n");
	}
	return 0;
}

int ph_getCmds(ph_db_table_t *ph_db_tables, int ph_db_tables_size,
		ph_mod_t *modules, xmlNodePtr mod_node)
{
	int i;
	int j;
	char *val;
	int val_len;
	ph_cmd_t *cmds;
	xmlNodePtr node, cmd_cols;
	str name;

	for(node=mod_node->children,modules->cmds_size=0;node;node=node->next){
		if (xmlStrcasecmp(node->name,
			(const xmlChar*)PI_HTTP_XML_CMD_NODE) == 0) {
			if(modules->cmds_size)
				cmds = (ph_cmd_t*)shm_realloc(modules->cmds,
					(modules->cmds_size+1)*sizeof(ph_cmd_t));
			else
				cmds = (ph_cmd_t*)shm_malloc(sizeof(ph_cmd_t));;
			if (cmds==NULL) {LM_ERR("oom\n");return -1;}
			modules->cmds = cmds;
			cmds = &modules->cmds[modules->cmds_size];
			memset(cmds, 0, sizeof(ph_cmd_t));
			cmds->type = -1;
			/* Populate the cmd name */
			name.s =
				ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_CMD_NAME_NODE);
			if(name.s==NULL){
				LM_ERR("no %s for %s [%.*s]\n",
					PI_HTTP_XML_CMD_NAME_NODE,
					mod_node->name,
					modules->module.len, modules->module.s);
				return -1;
			}
			name.len = strlen(name.s);
			if(name.len==0){
				LM_ERR("empty %s for %s [%.*s]\n",
					PI_HTTP_XML_CMD_NAME_NODE,
					mod_node->name,
					modules->module.len, modules->module.s);
				return -1;
			}
			if(shm_str_dup(&cmds->name, &name)!=0) return -1;
			xmlFree(name.s);name.s=NULL;name.len=0;
			/* Each cmd name MUST be unique */
			for(i=0;i<modules->cmds_size;i++){
				if(cmds->name.len==modules->cmds[i].name.len &&
                                        strncmp(modules->cmds[i].name.s,
                                                cmds->name.s,
						cmds->name.len)==0){
					LM_ERR("duplicated %s %s %s [%.*s]\n",
						mod_node->name, modules->module.s,
						node->name,
						cmds->name.len, cmds->name.s);
					return -1;
				}
			}
			/* Populate the db_table_index */
			val = ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_DB_TABLE_ID_NODE);
			if(val==NULL){
				LM_ERR("no %s for %s [%.*s] %s [%.*s]\n",
					PI_HTTP_XML_DB_TABLE_ID_NODE,
					mod_node->name,
					modules->module.len, modules->module.s,
					PI_HTTP_XML_CMD_NODE,
					cmds->name.len, cmds->name.s);
				return -1;
			}
			val_len = strlen(val);
			if(val_len==0){
				LM_ERR("empty %s for %s [%.*s] %s [%.*s]\n",
					PI_HTTP_XML_DB_URL_ID_NODE,
					mod_node->name,
					modules->module.len, modules->module.s,
					PI_HTTP_XML_CMD_NODE,
					cmds->name.len, cmds->name.s);
				xmlFree(val);
				return -1;
			}
			/* Get db_table */
			for(i=0;i<ph_db_tables_size;i++){
				if(val_len==ph_db_tables[i].id.len &&
					strncmp(ph_db_tables[i].id.s,
						val, val_len)==0){
					cmds->db_table = &ph_db_tables[i];
					break;
				}
			}
			if (i==ph_db_tables_size){
				LM_ERR("bogus %s [%s] for %s [%.*s] %s [%.*s]\n",
					PI_HTTP_XML_DB_TABLE_ID_NODE, val,
					mod_node->name,
					modules->module.len, modules->module.s,
					PI_HTTP_XML_CMD_NODE,
					cmds->name.len, cmds->name.s);
				xmlFree(val);
				return -1;
			}
			xmlFree(val);
			/* Get cmd_type */
			val = ph_xmlNodeGetNodeContentByName(node->children,
						PI_HTTP_XML_CMD_TYPE_NODE);
			if(val==NULL){
				LM_ERR("no %s for %s [%.*s] %s [%.*s]\n",
					PI_HTTP_XML_CMD_TYPE_NODE,
					mod_node->name,
					modules->module.len, modules->module.s,
					PI_HTTP_XML_CMD_NODE,
					cmds->name.len, cmds->name.s);
				return -1;
			}
			val_len = strlen(val);
			if(val_len==0){
				LM_ERR("empty %s for %s [%.*s] %s [%.*s]\n",
					PI_HTTP_XML_CMD_TYPE_NODE,
					mod_node->name,
					modules->module.len, modules->module.s,
					PI_HTTP_XML_CMD_NODE,
					cmds->name.len, cmds->name.s);
				xmlFree(val);
				return -1;
			}else if(val_len==8){
				if(strncmp("DB_QUERY",val,8)==0){
					cmds->type = DB_CAP_QUERY;
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_CLAUSE_COLS_NODE);
					if(cmd_cols!=NULL)
						if (ph_getCols( modules,
								cmds,
								&cmds->c_ops,
								&cmds->c_keys,
								&cmds->c_types,
								&cmds->c_vals,
								NULL,
								&cmds->c_keys_size,
								cmd_cols)!=0)
							return -1;
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_QUERY_COLS_NODE);
					if(cmd_cols!=NULL){
						if (ph_getCols( modules,
								cmds,
								NULL,
								&cmds->q_keys,
								&cmds->q_types,
								NULL,
								&cmds->link_cmd,
								&cmds->q_keys_size,
								cmd_cols)!=0)
							return -1;
					}else{
						LM_ERR("no %s in %s [%.*s] %s [%.*s]\n",
							PI_HTTP_XML_QUERY_COLS_NODE,
							mod_node->name,
							modules->module.len,
							modules->module.s,
							node->name,
							cmds->name.len,
							cmds->name.s);
						return -1;
					}
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_ORDER_BY_COLS_NODE);
					if(cmd_cols!=NULL){
						if (ph_getCols( modules,
								cmds,
								NULL,
								&cmds->o_keys,
								NULL,
								NULL,
								NULL,
								&cmds->o_keys_size,
								cmd_cols)!=0)
							return -1;
					}
				}
			}else if(val_len==9){
				if(strncmp("DB_INSERT",val,9)==0){
					cmds->type = DB_CAP_INSERT;
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_QUERY_COLS_NODE);
					if(cmd_cols!=NULL){
						if (ph_getCols( modules,
								cmds,
								NULL,
								&cmds->q_keys,
								&cmds->q_types,
								&cmds->q_vals,
								NULL,
								&cmds->q_keys_size,
								cmd_cols)!=0)
							return -1;
					}else{
						LM_ERR("no %s in %s [%.*s] %s [%.*s]\n",
							PI_HTTP_XML_QUERY_COLS_NODE,
							mod_node->name,
							modules->module.len,
							modules->module.s,
							node->name,
							cmds->name.len,
							cmds->name.s);
						return -1;
					}
				}else if(strncmp("DB_DELETE",val,9)==0){
					cmds->type = DB_CAP_DELETE;
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_CLAUSE_COLS_NODE);
					if(cmd_cols!=NULL){
						if (ph_getCols( modules,
								cmds,
								&cmds->c_ops,
								&cmds->c_keys,
								&cmds->c_types,
								&cmds->c_vals,
								NULL,
								&cmds->c_keys_size,
								cmd_cols)!=0)
							return -1;
					}else{
						LM_ERR("no %s in %s [%.*s] %s [%.*s]\n",
							PI_HTTP_XML_CLAUSE_COLS_NODE,
							mod_node->name,
							modules->module.len,
							modules->module.s,
							node->name,
							cmds->name.len,
							cmds->name.s);
						return -1;
					}

				}else if(strncmp("DB_UPDATE",val,9)==0){
					cmds->type = DB_CAP_UPDATE;
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_CLAUSE_COLS_NODE);
					if(cmd_cols!=NULL)
						if (ph_getCols( modules,
								cmds,
								&cmds->c_ops,
								&cmds->c_keys,
								&cmds->c_types,
								&cmds->c_vals,
								NULL,
								&cmds->c_keys_size,
								cmd_cols)!=0)
							return -1;
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_QUERY_COLS_NODE);
					if(cmd_cols!=NULL){
						if (ph_getCols( modules,
								cmds,
								NULL,
								&cmds->q_keys,
								&cmds->q_types,
								&cmds->q_vals,
								NULL,
								&cmds->q_keys_size,
								cmd_cols)!=0)
							return -1;
					}else{
						LM_ERR("no %s in %s [%.*s] %s [%.*s]\n",
							PI_HTTP_XML_QUERY_COLS_NODE,
							mod_node->name,
							modules->module.len,
							modules->module.s,
							node->name,
							cmds->name.len,
							cmds->name.s);
						return -1;
					}
				}
			}else if(val_len==10){
				if(strncmp("DB_REPLACE",val,10)==0){
					cmds->type = DB_CAP_REPLACE;
					cmd_cols =
					ph_xmlNodeGetNodeByName(node->children,
						PI_HTTP_XML_QUERY_COLS_NODE);
					if(cmd_cols!=NULL){
						if (ph_getCols( modules,
								cmds,
								NULL,
								&cmds->q_keys,
								&cmds->q_types,
								&cmds->q_vals,
								NULL,
								&cmds->q_keys_size,
								cmd_cols)!=0)
							return -1;
					}else{
						LM_ERR("no %s in %s [%.*s] %s [%.*s]\n",
							PI_HTTP_XML_QUERY_COLS_NODE,
							mod_node->name,
							modules->module.len,
							modules->module.s,
							node->name,
							cmds->name.len,
							cmds->name.s);
						return -1;
					}
				}
			}
			if(cmds->type==-1){
				LM_ERR("unexpected type [%s] for %s [%.*s] %s [%.*s]\n",
					val, mod_node->name,
					modules->module.len,
					modules->module.s,
					PI_HTTP_XML_CMD_NODE,
					cmds->name.len, cmds->name.s);
				xmlFree(val);
				return -1;
			}
			xmlFree(val);
			/**/
			LM_DBG("got node %s %s %s [%.*s] [%p]=>[%.*s]\n",
					mod_node->name, node->name,
					PI_HTTP_XML_CMD_NAME_NODE,
					cmds->name.len, cmds->name.s,
					cmds->db_table->name.s,
					cmds->db_table->name.len,
					cmds->db_table->name.s);
			if(cmds->c_keys)
				for(i=0;i<cmds->c_keys_size;i++){
					LM_DBG("    [%d] c_keys=[%.*s] "
						"c_ops=[%s] c_types=[%d]\n",
						i, (*(cmds->c_keys[i])).len,
						(*(cmds->c_keys[i])).s,
						cmds->c_ops[i], cmds->c_types[i]);
					if(cmds->c_vals)
						for(j=0;j<cmds->c_vals->vals_size;j++)
							LM_DBG("      c_vals[%d] "
							"id=[%.*s] val=[%.*s]\n",
							j,
							cmds->c_vals->ids->len,
							cmds->c_vals->ids->s,
							cmds->c_vals->vals->len,
							cmds->c_vals->vals->s);
				}
			if(cmds->q_keys)
				for(i=0;i<cmds->q_keys_size;i++){
					LM_DBG("    [%d] q_keys=[%.*s] "
						"q_types=[%d] link_cmd=[%.*s]\n",
						i, (*(cmds->q_keys[i])).len,
						(*(cmds->q_keys[i])).s,
						cmds->q_types[i],
						(cmds->link_cmd)?(cmds->link_cmd[i]).len:0,
						(cmds->link_cmd)?(cmds->link_cmd[i]).s:NULL);
					if(cmds->q_vals)
						for(j=0;j<cmds->q_vals->vals_size;j++)
							LM_DBG("      c_vals[%d] "
							"id=[%.*s] val=[%.*s]\n",
							j,
							cmds->q_vals->ids->len,
							cmds->q_vals->ids->s,
							cmds->q_vals->vals->len,
							cmds->q_vals->vals->s);
				}
			if(cmds->o_keys)
				for(i=0;i<cmds->o_keys_size;i++)
					LM_DBG("    o_keys[%d]=[%.*s]\n",
						i, (*(cmds->o_keys[i])).len,
						(*(cmds->o_keys[i])).s);
			/**/
			modules->cmds_size++;
		}
	}
	return 0;
}

int ph_getMods(ph_framework_t *_ph_framework_data, xmlNodePtr framework_node)
{
	int i;
	ph_mod_t *modules;
	xmlNodePtr mod_node;
	ph_mod_t *ph_modules;
	str module;

	/* Build pi commands skeleton */
	for(mod_node=framework_node->children,_ph_framework_data->ph_modules_size=0;
					mod_node;mod_node=mod_node->next){
		if (xmlStrcasecmp(mod_node->name,
			(const xmlChar*)PI_HTTP_XML_MOD_NODE) == 0) {
			if(_ph_framework_data->ph_modules_size)
				modules = (ph_mod_t*)shm_realloc(_ph_framework_data->ph_modules,
					(_ph_framework_data->ph_modules_size+1)*
					sizeof(ph_mod_t));
			else
				modules = (ph_mod_t*)shm_malloc(sizeof(ph_mod_t));
			if(modules==NULL){
				LM_ERR("oom\n");
				return -1;
			}
			_ph_framework_data->ph_modules = modules;
			ph_modules = _ph_framework_data->ph_modules;
			modules = &ph_modules[_ph_framework_data->ph_modules_size];
			memset(modules, 0, sizeof(ph_mod_t));

			/* Populate module name */
			module.s =
				ph_xmlNodeGetNodeContentByName(mod_node->children,
							PI_HTTP_XML_MOD_NAME_NODE);
			if(module.s==NULL){
				LM_ERR("no %s for node %s\n",
					PI_HTTP_XML_MOD_NAME_NODE,
					PI_HTTP_XML_MOD_NODE);
				return -1;
			}
			module.len = strlen(module.s);
			if(module.len==0){
				LM_ERR("empty %s for node %s\n",
					PI_HTTP_XML_MOD_NAME_NODE,
					PI_HTTP_XML_MOD_NODE);
				return -1;
			}
			if(shm_str_dup(&modules->module, &module)!=0) return -1;
			xmlFree(module.s);module.s=NULL;module.len=0;
			/* Each mod name MUST be unique */
			for(i=0;i<_ph_framework_data->ph_modules_size;i++){
				if(modules->module.len==ph_modules[i].module.len &&
                                        strncmp(ph_modules[i].module.s,
                                                modules->module.s,
						modules->module.len)==0){
					LM_ERR("duplicated %s [%.*s]\n",
						mod_node->name,
						modules->module.len,
						modules->module.s);
					return -1;
				}
			}
			/* Get cmds */
			if(ph_getCmds(_ph_framework_data->ph_db_tables,
					_ph_framework_data->ph_db_tables_size,
					modules, mod_node)!=0)
				return -1;
			_ph_framework_data->ph_modules_size++;
			LM_DBG("got node %s [%.*s]\n",
				mod_node->name,
				modules->module.len, modules->module.s);
		}
	}
	if(_ph_framework_data->ph_modules_size==0){
		LM_ERR("no %s node in config file\n", PI_HTTP_XML_MOD_NODE);
		return -1;
	}
	return 0;
}


void ph_freeDbUrlNodes(ph_db_url_t **ph_db_urls, int ph_db_urls_size)
{
	int i;
	ph_db_url_t *_ph_db_urls = *ph_db_urls;

	if(_ph_db_urls==NULL) return;
	for(i=0;i<ph_db_urls_size;i++){
		shm_free(_ph_db_urls[i].id.s);
		_ph_db_urls[i].id.s = NULL;
		shm_free(_ph_db_urls[i].db_url.s);
		_ph_db_urls[i].db_url.s = NULL;
	}
	shm_free(*ph_db_urls); *ph_db_urls = NULL;
	return;
}
void ph_freeDbTables(ph_db_table_t **ph_db_tables, int ph_db_tables_size)
{
	int i, j;
	ph_db_table_t *_ph_db_tables = *ph_db_tables;

	if(_ph_db_tables==NULL) return;
	for(i=0;i<ph_db_tables_size;i++){
		shm_free(_ph_db_tables[i].id.s);
		_ph_db_tables[i].id.s = NULL;
		shm_free(_ph_db_tables[i].name.s);
		_ph_db_tables[i].name.s = NULL;
		for(j=0;j<_ph_db_tables[i].cols_size;j++){
			shm_free(_ph_db_tables[i].cols[j].field.s);
			_ph_db_tables[i].cols[j].field.s = NULL;
		}
		shm_free(_ph_db_tables[i].cols);
		_ph_db_tables[i].cols = NULL;
	}
	shm_free(*ph_db_tables); *ph_db_tables = NULL;
	return;
}
void ph_freeMods(ph_mod_t **ph_modules, int ph_modules_size)
{
	int i, j, k;
	ph_mod_t *_ph_modules = *ph_modules;
	db_key_t *cmd_keys;
	db_op_t *cmd_ops;
	ph_vals_t *cmd_vals;
	str *cmd_linkCmd;

	if(_ph_modules==NULL) return;
	for(i=0;i<ph_modules_size;i++){
		if(_ph_modules[i].module.s){
			shm_free(_ph_modules[i].module.s);
			_ph_modules[i].module.s = NULL;
		}
		for(j=0;j<_ph_modules[i].cmds_size;j++){
			if(_ph_modules[i].cmds[j].name.s){
				shm_free(_ph_modules[i].cmds[j].name.s);
				_ph_modules[i].cmds[j].name.s = NULL;
			}
			/* */
			cmd_keys = _ph_modules[i].cmds[j].c_keys;
			cmd_ops = _ph_modules[i].cmds[j].c_ops;
			cmd_vals = _ph_modules[i].cmds[j].c_vals;
			for(k=0;k<_ph_modules[i].cmds[j].c_keys_size;k++){
				if(cmd_ops && cmd_ops[k]){
					shm_free((char*)cmd_ops[k]);
					cmd_ops[k] = NULL;
				}
				if(cmd_keys && cmd_keys[k]){
					if(cmd_keys[k]->s){
						shm_free(cmd_keys[k]->s);
						cmd_keys[k]->s = NULL;
					}
					shm_free(cmd_keys[k]);
					cmd_keys[k] = NULL;
				}
				if(cmd_vals){
					if(cmd_vals[k].ids){
						if(cmd_vals[k].ids->s){
							shm_free(cmd_vals[k].ids->s);
							cmd_vals[k].ids->s = NULL;
						}
						shm_free(cmd_vals[k].ids);
						cmd_vals[k].ids = NULL;
					}
					if(cmd_vals[k].vals){
						if(cmd_vals[k].vals->s){
							shm_free(cmd_vals[k].vals->s);
							cmd_vals[k].vals->s = NULL;
						}
						shm_free(cmd_vals[k].vals);
						cmd_vals[k].vals = NULL;
					}
				}
			}
			if(_ph_modules[i].cmds[j].c_keys){
				shm_free(_ph_modules[i].cmds[j].c_keys);
				_ph_modules[i].cmds[j].c_keys = NULL;
			}
			if(_ph_modules[i].cmds[j].c_ops){
				shm_free(_ph_modules[i].cmds[j].c_ops);
				_ph_modules[i].cmds[j].c_ops = NULL;
			}
			if(_ph_modules[i].cmds[j].c_types){
				shm_free(_ph_modules[i].cmds[j].c_types);
				_ph_modules[i].cmds[j].c_types = NULL;
			}
			if(_ph_modules[i].cmds[j].c_vals){
				shm_free(_ph_modules[i].cmds[j].c_vals);
				_ph_modules[i].cmds[j].c_vals = NULL;
			}
			/* */
			cmd_keys = _ph_modules[i].cmds[j].q_keys;
			cmd_vals = _ph_modules[i].cmds[j].q_vals;
			cmd_linkCmd = _ph_modules[i].cmds[j].link_cmd;
			for(k=0;k<_ph_modules[i].cmds[j].q_keys_size;k++){
				if(cmd_keys && cmd_keys[k]){
					if(cmd_keys[k]->s){
						shm_free(cmd_keys[k]->s);
						cmd_keys[k]->s = NULL;
					}
					shm_free(cmd_keys[k]);
					cmd_keys[k] = NULL;
				}
				if(cmd_vals){
					if(cmd_vals[k].ids){
						if(cmd_vals[k].ids->s){
							shm_free(cmd_vals[k].ids->s);
							cmd_vals[k].ids->s = NULL;
						}
						shm_free(cmd_vals[k].ids);
						cmd_vals[k].ids = NULL;
					}
					if(cmd_vals[k].vals){
						if(cmd_vals[k].vals->s){
							shm_free(cmd_vals[k].vals->s);
							cmd_vals[k].vals->s = NULL;
						}
						shm_free(cmd_vals[k].vals);
						cmd_vals[k].vals = NULL;
					}
				}
				if(cmd_linkCmd && cmd_linkCmd[k].s){
					shm_free(cmd_linkCmd[k].s);
					cmd_linkCmd[k].s = NULL;
				}
			}
			if(_ph_modules[i].cmds[j].q_keys){
				shm_free(_ph_modules[i].cmds[j].q_keys);
				_ph_modules[i].cmds[j].q_keys = NULL;
			}
			if(_ph_modules[i].cmds[j].q_types){
				shm_free(_ph_modules[i].cmds[j].q_types);
				_ph_modules[i].cmds[j].q_types = NULL;
			}
			if(_ph_modules[i].cmds[j].q_vals){
				shm_free(_ph_modules[i].cmds[j].q_vals);
				_ph_modules[i].cmds[j].q_vals = NULL;
			}
			if(_ph_modules[i].cmds[j].link_cmd){
				shm_free(_ph_modules[i].cmds[j].link_cmd);
				_ph_modules[i].cmds[j].link_cmd = NULL;
			}
			cmd_keys = NULL; cmd_vals = NULL;
			/* */
			cmd_keys = _ph_modules[i].cmds[j].c_keys;
			for(k=0;k<_ph_modules[i].cmds[j].c_keys_size;k++){
				if(cmd_keys && cmd_keys[k]){
					if(cmd_keys[k]->s){
						shm_free(cmd_keys[k]->s);
						cmd_keys[k]->s = NULL;
					}
					shm_free(cmd_keys[k]);
					cmd_keys[k] = NULL;
				}
			}
			if(_ph_modules[i].cmds[j].c_keys){
				shm_free(_ph_modules[i].cmds[j].c_keys);
				_ph_modules[i].cmds[j].c_keys = NULL;
			}
			cmd_keys = NULL;
		}
		if(_ph_modules[i].cmds){
			shm_free(_ph_modules[i].cmds);
			_ph_modules[i].cmds = NULL;
		}
	}
	if(*ph_modules){
		shm_free(*ph_modules); *ph_modules = NULL;
	}
	return;
}

int ph_init_cmds(ph_framework_t **framework_data, const char* filename)
{
	xmlDocPtr doc;
	xmlNodePtr framework_node;
	ph_framework_t *_framework_data = NULL;
	ph_db_table_t *_ph_db_tables;
	int _ph_db_tables_size;
	ph_mod_t *_ph_modules;
	int _ph_modules_size;

	if(filename==NULL) {LM_ERR("NULL filename\n");return -1;}
	doc = xmlParseFile(filename);
	if(doc==NULL){
		LM_ERR("Failed to parse xml file: %s\n", filename);
		return -1;
	}

	/* Extract the framework node */
	framework_node = ph_xmlNodeGetNodeByName(doc->children,
						PI_HTTP_XML_FRAMEWORK_NODE);
	if (framework_node==NULL) {
		LM_ERR("missing node %s\n", PI_HTTP_XML_FRAMEWORK_NODE);
		goto xml_error;
	}

	_framework_data = *framework_data;
	if(_framework_data==NULL){
		_framework_data =
		(ph_framework_t*)shm_malloc(sizeof(ph_framework_t));
		if(_framework_data==NULL) {LM_ERR("oom\n");goto xml_error;}
		memset(_framework_data, 0, sizeof(ph_framework_t));

		/* Extract the db_url nodes */
		if(ph_getDbUrlNodes(_framework_data, framework_node)!=0)
			goto xml_error;

		/* Extract the db_url nodes */
		if(ph_getDbTables(_framework_data, framework_node)!=0)
			goto xml_error;

		/* Build pi commands skeleton */
		if(ph_getMods(_framework_data, framework_node)!=0)
			goto xml_error;

		if (doc) {
			xmlFree(doc);
			doc = NULL;
		}
		*framework_data = _framework_data;
	}else{ /* This is a reload */
		_ph_db_tables = _framework_data->ph_db_tables;
		_ph_db_tables_size = _framework_data->ph_db_tables_size;
		_framework_data->ph_db_tables = NULL;
		_framework_data->ph_db_tables_size = 0;
		_ph_modules = _framework_data->ph_modules;
		_ph_modules_size = _framework_data->ph_modules_size;
		_framework_data->ph_modules = NULL;
		_framework_data->ph_modules_size = 0;

		/* Extract the db_url nodes */
		if(ph_getDbTables(_framework_data, framework_node)!=0)
			goto xml_reload_error;

		/* Build pi commands skeleton */
		if(ph_getMods(_framework_data, framework_node)!=0)
			goto xml_reload_error;

		if (doc) {
			xmlFree(doc);
			doc = NULL;
		}
		*framework_data = _framework_data;

	}
	return 0;
xml_error:
	/* FIXME: free thw whole structure */
	if(_framework_data){shm_free(_framework_data);}
	if (doc) {
		xmlFree(doc);
		doc = NULL;
	}
	return -1;
xml_reload_error:
	ph_freeDbTables(&_framework_data->ph_db_tables,
			_framework_data->ph_db_tables_size);
	ph_freeMods(&_framework_data->ph_modules,
			_framework_data->ph_modules_size);
	_framework_data->ph_db_tables = _ph_db_tables;
	_framework_data->ph_db_tables_size = _ph_db_tables_size;
	_framework_data->ph_modules = _ph_modules;
	_framework_data->ph_modules_size = _ph_modules_size;
	if (doc) {
		xmlFree(doc);
		doc = NULL;
	}
	return -1;
}



int ph_parse_url(const char* url, int* mod, int* cmd)
{
	int url_len = strlen(url);
	int index = 0;
	int i;
	int mod_len, cmd_len;
	ph_mod_t *ph_modules = ph_framework_data->ph_modules;


	if (url_len<0) {
		LM_ERR("Invalid url length [%d]\n", url_len);
		return -1;
	}
	if (url_len==0) return 0;
	if (url[0] != '/') {
		LM_ERR("URL starting with [%c] instead of'/'\n", *url);
		return -1;
	}
	index++;

	/* Looking for "mod" */
	if (index>=url_len)
		return 0;
	for(i=index;i<url_len && url[i]!='/';i++);
	mod_len = i - index;
	for(i=0;i<ph_framework_data->ph_modules_size &&
		(mod_len!=ph_modules[i].module.len ||
		strncmp(&url[index], ph_modules[i].module.s,mod_len)!=0);i++);
	if (i==ph_framework_data->ph_modules_size) {
		LM_ERR("Invalid mod [%.*s] in url [%s]\n",
			mod_len, &url[index], url);
		return -1;
	}
	*mod = i;
	LM_DBG("got mod [%d][%.*s]\n", *mod, mod_len, &url[index]);

	index += mod_len;
	LM_DBG("index=%d url_len=%d\n", index, url_len);
	if (index>=url_len)
		return 0;

	/* skip over '/' */
	index++;

	/* Looking for "cmd" */
	if (index>=url_len)
		return 0;
	for(i=index;i<url_len && url[i]!='/';i++);
	cmd_len = i - index;
	for(i=0;i<ph_modules[*mod].cmds_size &&
		(cmd_len!=ph_modules[*mod].cmds[i].name.len ||
		strncmp(&url[index], ph_modules[*mod].cmds[i].name.s, cmd_len)!=0);
		i++);
	if (i==ph_modules[*mod].cmds_size) {
		LM_ERR("Invalid cmd [%.*s] in url [%s]\n",
			cmd_len, &url[index], url);
		return -1;
	}
	*cmd = i;
	LM_DBG("got cmd [%d][%.*s]\n", *cmd, cmd_len, &url[index]);
	index += cmd_len;
	if (index>=url_len)
		return 0;
	/* skip over '/' */
	index++;
	if (url_len - index>0) {
		LM_DBG("got extra [%s]\n", &url[index]);
	}

	return 0;
}


int ph_build_form_imput(char **p, char *buf, str *page, int max_page_len,
		int mod, int cmd, str *clause, db_val_t *values)
{
	unsigned long i, j;
	char c;
	str op, arg;
	str val_str;
	str temp_holder;
	int temp_counter;
	ph_cmd_t *command;
	ph_mod_t *ph_modules;

	ph_modules = ph_framework_data->ph_modules;
	command = &ph_modules[mod].cmds[cmd];
	if(command->c_keys_size && (command->type==DB_CAP_QUERY ||
				command->type==DB_CAP_DELETE ||
				command->type==DB_CAP_UPDATE)){
		PI_HTTP_COPY_3(*p,PI_HTTP_Post_Input,
				PI_HTTP_Post_Clause_Input,
				PI_HTTP_Post_Query_Input);
		for(i=0;i<command->c_keys_size;i++){
			/* FIXME: we should escape c_ops */
			op.s = (char*)command->c_ops[i];
			op.len = strlen(op.s);
			arg.s = int2str(i, &arg.len);
			switch(command->c_vals[i].vals_size){
			case 0:
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_1);
				PI_HTTP_COPY(*p, *command->c_keys[i]);
				PI_HTTP_COPY(*p, PI_HTTP_ATTR_SEPARATOR);
				PI_HTTP_COPY(*p, op);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Text);
				PI_HTTP_COPY(*p, arg);
				if (i==0 && clause) {
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_2);
					PI_HTTP_COPY(*p, *clause);
				}
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_3);
				break;
			case 1:
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_1);
				PI_HTTP_COPY(*p, arg);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_2);
				PI_HTTP_COPY(*p, command->c_vals[i].vals[0]);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_3);
				break;
			default:
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_1);
				PI_HTTP_COPY(*p, *command->c_keys[i]);
				PI_HTTP_COPY(*p, PI_HTTP_ATTR_SEPARATOR);
				PI_HTTP_COPY(*p, op);
				LM_DBG("Here we need to enforce select\n");
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Select_1);
				PI_HTTP_COPY(*p, arg);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Select_2);
				for(j=0;j<command->c_vals[i].vals_size;j++){
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Option_1);
					PI_HTTP_COPY(*p, command->c_vals[i].vals[j]);
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Option_2);
					PI_HTTP_COPY(*p, command->c_vals[i].ids[j]);
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Option_3);
				}
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Select_3);
			}
		}
		PI_HTTP_COPY(*p, PI_HTTP_Post_Input_4);
	}
	if(command->q_keys_size && (command->type==DB_CAP_INSERT ||
				command->type==DB_CAP_UPDATE ||
				command->type==DB_CAP_REPLACE)){
		PI_HTTP_COPY_3(*p,PI_HTTP_Post_Input,
				PI_HTTP_Post_Values_Input,
				PI_HTTP_Post_Query_Input);
		arg.s = &c; arg.len = 1;
		for(i=0,c='a';i<command->q_keys_size;i++,c++){
			if(c=='z'){
				LM_ERR("To many q_keys\n"); return -1;
			}
			switch(command->q_vals[i].vals_size){
			case 0:
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_1);
				PI_HTTP_COPY(*p, *command->q_keys[i]);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Text);
				PI_HTTP_COPY(*p, arg);
				if (values) {
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_2);
					switch(command->q_types[i]){
					case DB_STR:
					case DB_STRING:
					case DB_BLOB:
						if(values[i].val.str_val.s==NULL){
							val_str.s = NULL; val_str.len = 0;
						} else {
							val_str.s = values[i].val.str_val.s;
							val_str.len = strlen(val_str.s);
						}
						LM_DBG("...got %.*s[0]=>"
							"[%.*s][%.*s]\n",
							command->q_keys[i]->len,
							command->q_keys[i]->s,
							values[i].val.str_val.len,
							values[i].val.str_val.s,
							val_str.len, val_str.s);
						if (val_str.len) {
							PI_HTTP_ESC_COPY(*p, val_str, temp_holder, temp_counter);
						}
						break;
					case DB_INT:
						val_str.s = *p;
						val_str.len = max_page_len - page->len;
						if(db_int2str(values[i].val.int_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert int [%d]\n",
								values[i].val.int_val);
							goto error;
						}
						*p += val_str.len;
						page->len += val_str.len;
						LM_DBG("   got %.*s[0]=>"
							"[%d][%.*s]\n",
							command->q_keys[i]->len,
							command->q_keys[i]->s,
							values[i].val.int_val,
							val_str.len, val_str.s);
						break;
					case DB_BITMAP:
						val_str.s = *p;
						val_str.len = max_page_len - page->len;
						if(db_int2str(values[i].val.bitmap_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert bitmap [%d]\n",
								values[i].val.bitmap_val);
							goto error;
						}
						*p += val_str.len;
						page->len += val_str.len;
						LM_DBG("   got %.*s[0]=>"
							"[%d][%.*s]\n",
							command->q_keys[i]->len,
							command->q_keys[i]->s,
							values[i].val.bitmap_val,
							val_str.len, val_str.s);
						break;
					case DB_BIGINT:
						val_str.s = *p;
						val_str.len = max_page_len - page->len;
						if(db_bigint2str(values[i].val.bigint_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert bigint [%-lld]\n",
								values[i].val.bigint_val);
							goto error;
						}
						*p += val_str.len;
						page->len += val_str.len;
						LM_DBG("   got %.*s[0]=>"
							"[%-lld][%.*s]\n",
							command->q_keys[i]->len,
							command->q_keys[i]->s,
							values[i].val.bigint_val,
							val_str.len, val_str.s);
						break;
					case DB_DOUBLE:
						val_str.s = *p;
						val_str.len = max_page_len - page->len;
						if(db_double2str(values[i].val.double_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert double [%-10.2f]\n",
								values[i].val.double_val);
							goto error;
						}
						*p += val_str.len;
						page->len += val_str.len;
						LM_DBG("   got %.*s[0]=>"
							"[%-10.2f][%.*s]\n",
							command->q_keys[i]->len,
							command->q_keys[i]->s,
							values[i].val.double_val,
							val_str.len, val_str.s);
						break;
					case DB_DATETIME:
						val_str.s = *p;
						val_str.len = max_page_len - page->len;
						if (db_time2str_nq(values[i].val.time_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert time [%ld]\n",
								(unsigned long int)values[i].val.time_val);
							goto error;
						}
						*p += val_str.len;
						page->len += val_str.len;
						LM_DBG("   got %.*s[0]=>"
							"[%ld][%.*s]\n",
							command->q_keys[i]->len,
							command->q_keys[i]->s,
							(unsigned long int)values[i].val.time_val,
							val_str.len, val_str.s);
						break;
					default:
						LM_ERR("unexpected type [%d] "
							"for [%.*s]\n",
							command->q_types[i],
							command->q_keys[i]->len,
							command->q_keys[i]->s);
					}
				}
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_3);
				break;
			case 1:
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_1);
				PI_HTTP_COPY(*p, arg);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_2);
				PI_HTTP_COPY(*p, command->q_vals[i].vals[0]);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Hidden_3);
				break;
			default:
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_1);
				PI_HTTP_COPY(*p, *command->q_keys[i]);
				LM_DBG("Here we need to enforce select\n");
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Select_1);
				PI_HTTP_COPY(*p, arg);
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Select_2);
				for(j=0;j<command->q_vals[i].vals_size;j++){
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Option_1);
					PI_HTTP_COPY(*p, command->q_vals[i].vals[j]);
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Option_2);
					PI_HTTP_COPY(*p, command->q_vals[i].ids[j]);
					PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Option_3);
				}
				PI_HTTP_COPY(*p, PI_HTTP_Post_Input_Select_3);
			}
		}
		PI_HTTP_COPY(*p, PI_HTTP_Post_Input_4);
	}
	return 0;
error:
	LM_ERR("buffer 2 small: *p=[%p] buf=[%p] max_page_len=[%d]\n",
			*p, buf, max_page_len);
	return -1;
}


int ph_build_header(str *page, int max_page_len, int mod, int cmd)
{
	int i;
	char *p, *buf;
	ph_mod_t *ph_modules;

	ph_modules = ph_framework_data->ph_modules;
	if (page->s == NULL) {
		LM_ERR("Please provide a valid page\n");
		return -1;
	}
	p = buf = page->s;

	PI_HTTP_COPY_4(p,PI_HTTP_Response_Head_1,
			PI_HTTP_Response_Head_2,
			PI_HTTP_Response_Title_Table_1,
			PI_HTTP_Response_Title_Table_3);
	/* Building module menu */
	PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Table_1);
	for(i=0;i<ph_framework_data->ph_modules_size;i++) {
		if(i!=mod) {
			PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Table_2);
		} else {
			PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Table_2b);
		}
		PI_HTTP_COPY(p,PI_HTTP_SLASH);
		if (http_root.len) {
			PI_HTTP_COPY_2(p,http_root,PI_HTTP_SLASH);
		}
		PI_HTTP_COPY_3(p,ph_modules[i].module,
				PI_HTTP_Response_Menu_Table_3,
				ph_modules[i].module);
		if(i!=mod) {
			PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Table_4);
		} else {
			PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Table_4b);
		}
	}
	PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Table_5);

	page->len = p - page->s;
	return 0;
error:
	LM_ERR("buffer 2 small\n");
	page->len = p - page->s;
	return -1;
}



int ph_build_reply(str *page, int max_page_len, int mod, int cmd)
{
	char *p, *buf;
	ph_mod_t *ph_modules;

	ph_modules = ph_framework_data->ph_modules;
	buf = page->s;
	p = page->s + page->len;

	/* Print comand name */
	PI_HTTP_COPY_4(p,PI_HTTP_Response_Menu_Cmd_Table_1b,
			PI_HTTP_Response_Menu_Cmd_tr_1,
			PI_HTTP_Response_Menu_Cmd_td_1a,
			PI_HTTP_SLASH);
	if (http_root.len) {
		PI_HTTP_COPY_2(p,http_root, PI_HTTP_SLASH);
	}
	PI_HTTP_COPY_6(p,ph_modules[mod].module,
			PI_HTTP_SLASH,
			ph_modules[mod].cmds[cmd].name,
			PI_HTTP_SQUOT_GT,
			ph_modules[mod].cmds[cmd].name,
			PI_HTTP_Response_Menu_Cmd_td_4a);
	/* Print cmd name */
	PI_HTTP_COPY_9(p,PI_HTTP_Response_Menu_Cmd_td_1e,
			ph_modules[mod].cmds[cmd].name,
			PI_HTTP_Response_Menu_Cmd_td_4d,
			PI_HTTP_Response_Menu_Cmd_tr_2,
			PI_HTTP_Response_Menu_Cmd_tr_1,
			PI_HTTP_Response_Menu_Cmd_td_1f,
			PI_HTTP_NBSP,
			PI_HTTP_Response_Menu_Cmd_td_4d,
			PI_HTTP_Response_Menu_Cmd_td_1d);

	page->len = p - page->s;
	return 0;
error:
	LM_ERR("buffer 2 small\n");
	page->len = p - page->s;
	return -1;
}

int ph_build_reply_footer(str *page, int max_page_len)
{
	char *p, *buf;
	/* Here we print the footer */
	buf = page->s;
	p = page->s + page->len;
	PI_HTTP_COPY_3(p,PI_HTTP_Response_Menu_Cmd_tr_2,
			PI_HTTP_Response_Menu_Cmd_Table_2,
			PI_HTTP_Response_Foot);
	page->len = p - page->s;
	return 0;
error:
	LM_ERR("buffer 2 small\n");
	page->len = p - page->s;
	return -1;
}

int ph_build_content(str *page, int max_page_len, int mod, int cmd, str *clause, db_val_t *values)
{
	char *p, *buf;
	int j;
	ph_mod_t *ph_modules;

	ph_modules = ph_framework_data->ph_modules;
	buf = page->s;
	p = page->s + page->len;

	if (mod>=0) { /* Building command menu */
		/* Build the list of comands for the selected module */
		PI_HTTP_COPY_4(p,PI_HTTP_Response_Menu_Cmd_Table_1a,
				PI_HTTP_Response_Menu_Cmd_tr_1,
				PI_HTTP_Response_Menu_Cmd_td_1a,
				PI_HTTP_SLASH);
		if (http_root.len) {
			PI_HTTP_COPY_2(p,http_root,PI_HTTP_SLASH);
		}
		PI_HTTP_COPY_6(p,ph_modules[mod].module,
				PI_HTTP_SLASH,
				ph_modules[mod].cmds[0].name,
				PI_HTTP_SQUOT_GT,
				ph_modules[mod].cmds[0].name,
				PI_HTTP_Response_Menu_Cmd_td_4a);
		if (cmd>=0) {
			PI_HTTP_COPY_3(p,PI_HTTP_Response_Menu_Cmd_td_1b,
					ph_modules[mod].cmds[cmd].name,
					PI_HTTP_Response_Menu_Cmd_td_4b);
		}
		PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_tr_2);
		for(j=1;j<ph_modules[mod].cmds_size;j++) {
			PI_HTTP_COPY_3(p,PI_HTTP_Response_Menu_Cmd_tr_1,
					PI_HTTP_Response_Menu_Cmd_td_1a,
					PI_HTTP_SLASH);
			if (http_root.len) {
				PI_HTTP_COPY_2(p,http_root, PI_HTTP_SLASH);
			}
			PI_HTTP_COPY_6(p,ph_modules[mod].module,
					PI_HTTP_SLASH,
					ph_modules[mod].cmds[j].name,
					PI_HTTP_SQUOT_GT,
					ph_modules[mod].cmds[j].name,
					PI_HTTP_Response_Menu_Cmd_td_4a);
			if (cmd>=0){
				if (j==1) {
					PI_HTTP_COPY_6(p,
						PI_HTTP_Response_Menu_Cmd_td_1c,
						PI_HTTP_CMD_ROWSPAN,
						PI_HTTP_Response_Menu_Cmd_td_3c,
						PI_HTTP_Post_Form_1a,
						PI_HTTP_METHOD[http_method],
						PI_HTTP_Post_Form_1b);
					if(ph_build_form_imput(&p, buf, page, max_page_len,
							mod, cmd, clause, values)!=0)
						return -1;
					PI_HTTP_COPY_2(p, PI_HTTP_Post_Form_2,
						PI_HTTP_Response_Menu_Cmd_td_4c);
				} else if (j>PI_HTTP_ROWSPAN) {
					PI_HTTP_COPY_3(p,
						PI_HTTP_Response_Menu_Cmd_td_1d,
						PI_HTTP_NBSP,
						PI_HTTP_Response_Menu_Cmd_td_4d);
				}
			}
			PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_tr_2);
		}
		if (cmd>=0){
			if (j==1) {
				PI_HTTP_COPY_10(p,PI_HTTP_Response_Menu_Cmd_tr_1,
						PI_HTTP_Response_Menu_Cmd_td_1d,
						PI_HTTP_NBSP,
						PI_HTTP_Response_Menu_Cmd_td_4d,
						PI_HTTP_Response_Menu_Cmd_td_1c,
						PI_HTTP_CMD_ROWSPAN,
						PI_HTTP_Response_Menu_Cmd_td_3c,
						PI_HTTP_Post_Form_1a,
						PI_HTTP_METHOD[http_method],
						PI_HTTP_Post_Form_1b);
				if(ph_build_form_imput(&p, buf, page, max_page_len,
						mod, cmd, clause, values)!=0)
					return -1;
				PI_HTTP_COPY_3(p, PI_HTTP_Post_Form_2,
						PI_HTTP_Response_Menu_Cmd_td_4c,
						PI_HTTP_Response_Menu_Cmd_tr_2);
				j++;
			}
			for(;j<=PI_HTTP_ROWSPAN;j++) {
				PI_HTTP_COPY_5(p,PI_HTTP_Response_Menu_Cmd_tr_1,
						PI_HTTP_Response_Menu_Cmd_td_1d,
						PI_HTTP_NBSP,
						PI_HTTP_Response_Menu_Cmd_td_4d,
						PI_HTTP_Response_Menu_Cmd_tr_2);
			}
		}
		PI_HTTP_COPY_2(p,PI_HTTP_Response_Menu_Cmd_Table_2,
				PI_HTTP_Response_Foot);
	} else {
		PI_HTTP_COPY(p,PI_HTTP_Response_Foot);
	}

	page->len = p - page->s;
	return 0;
error:
	LM_ERR("buffer 2 small\n");
	page->len = p - page->s;
	return -1;
}


int getVal(db_val_t *val, db_type_t val_type, db_key_t key, ph_db_table_t *table,
	str *arg, str *page, str *buffer, int mod, int cmd)
{
	char *p = page->s + page->len;
	char *buf = page->s;
	int _len, i;
	int max_page_len = buffer->len;
	ph_val_flags flags;

	str host;
	int port, proto;
	struct sip_uri uri;
	char c;

	for(i=0;i<=table->cols_size;i++){
		if(table->cols[i].type==val_type &&
			table->cols[i].field.len==key->len &&
			strncmp(table->cols[i].field.s,key->s,key->len)==0){
			if(table->cols[i].validation==0) continue;
			flags = table->cols[i].validation;
			LM_DBG("[%.*s] has flags [%u]\n", key->len, key->s, flags);
			if(flags&PH_FLAG_P_HOST_PORT){
				flags&= ~ PH_FLAG_P_HOST_PORT;
				if (parse_phostport(arg->s, arg->len,
						&host.s, &host.len,
						&port, &proto)!=0){
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Invalid [proto:]host[:port] for"
						"%.*s [%.*s].",
						key->len, key->s, arg->len,  arg->s);
					goto done;
				}
				LM_DBG("[%.*s]->[%d][%.*s][%d]\n",
					arg->len, arg->s, proto,
					host.len, host.s, port);
				continue;
			}
			LM_DBG("[%.*s] has flags [%d]\n", key->len, key->s, flags);
			if(flags&PH_FLAG_P_IPV4_PORT){
				flags&= ~ PH_FLAG_P_IPV4_PORT;
				if (parse_phostport(arg->s, arg->len,
						&host.s, &host.len,
						&port, &proto)!=0){
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Invalid [proto:]IPv4[:port] for"
						" %.*s [%.*s].",
						key->len, key->s, arg->len,  arg->s);
					goto done;
				}
				LM_DBG("[%.*s]->[%d][%.*s][%d]\n",
					arg->len, arg->s, proto,
					host.len, host.s, port);
				if (str2ip(&host)==NULL) {
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Invalid IPv4 in [proto:]IPv4[:port]"
						" %.*s [%.*s][%.*s].",
						key->len, key->s,
						host.len, host.s,
						arg->len, arg->s);
					goto done;
				}
				continue;
			}
			LM_DBG("[%.*s] has flags [%d]\n", key->len, key->s, flags);
			if(flags&PH_FLAG_IPV4){
				flags&= ~ PH_FLAG_IPV4;
				if (str2ip(arg)==NULL) {
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Invalid IPv4 for %.*s [%.*s].",
						key->len, key->s, arg->len, arg->s);
					goto done;
				}
				continue;
			}
			LM_DBG("[%.*s] has flags [%d]\n", key->len, key->s, flags);
			if(flags&PH_FLAG_URI){
				flags&= ~ PH_FLAG_URI;
				if (parse_uri(arg->s, arg->len, &uri)<0){
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Invalid SIP URI for %.*s [%.*s].",
						key->len, key->s, arg->len, arg->s);
					goto done;
				}
				continue;
			}
			LM_DBG("[%.*s] has flags [%d]\n", key->len, key->s, flags);
			if(flags&PH_FLAG_URI_IPV4HOST){
				flags&= ~ PH_FLAG_URI_IPV4HOST;
				if (parse_uri(arg->s, arg->len, &uri)<0){
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Invalid SIP URI for %.*s [%.*s].",
						key->len, key->s, arg->len, arg->s);
					goto done;
				}
				if (str2ip(&uri.host)==NULL) {
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Invalid IPv4 host in SIP URI for"
						" %.*s [%.*s][%.*s].",
						key->len, key->s,
						uri.host.len, uri.host.s,
						arg->len, arg->s);
					goto done;
				}
				continue;
			}
			LM_DBG("[%.*s] has flags [%d]\n", key->len, key->s, flags);
			if(flags){
				PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
					"Unkown validation [%d] for %s.",
					table->cols[i].validation, key->s);
				goto done;
			}
		}
	}
	switch(val_type){
	case DB_STR:
	case DB_STRING:
	case DB_BLOB:
		if(arg->len){
			val->val.str_val.s = arg->s;
			val->val.str_val.len = arg->len;
		}
		break;
	case DB_INT:
		c = arg->s[arg->len];
		arg->s[arg->len] = '\0';
		if(db_str2int(arg->s,&val->val.int_val)<0){
			arg->s[arg->len] = c;
			PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
				"Bogus field %.*s [%.*s].",
				key->len, key->s, arg->len, arg->s);
			goto done;
		}
		arg->s[arg->len] = c;
		break;
	case DB_BITMAP:
		c = arg->s[arg->len];
		arg->s[arg->len] = '\0';
		if(db_str2int(arg->s,(int*)&val->val.bitmap_val)<0){
			arg->s[arg->len] = c;
			PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
				"Bogus field %.*s [%.*s].",
				key->len, key->s, arg->len, arg->s);
			goto done;
		}
		arg->s[arg->len] = c;
		break;
	case DB_BIGINT:
		c = arg->s[arg->len];
		arg->s[arg->len] = '\0';
		if(db_str2bigint(arg->s,&val->val.bigint_val)<0){
			arg->s[arg->len] = c;
			PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
				"Bogus field %.*s [%.*s].",
				key->len, key->s, arg->len, arg->s);
			goto done;
		}
		arg->s[arg->len] = c;
		break;
	case DB_DOUBLE:
		c = arg->s[arg->len];
		arg->s[arg->len] = '\0';
		if(db_str2double(arg->s,&val->val.double_val)<0){
			arg->s[arg->len] = c;
			PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
				"Bogus field %.*s [%.*s].",
				key->len, key->s, arg->len, arg->s);
			goto done;
		}
		arg->s[arg->len] = c;
		break;
	case DB_DATETIME:
		c = arg->s[arg->len];
		arg->s[arg->len] = '\0';
		if(db_str2time(arg->s,&val->val.time_val)<0){
			arg->s[arg->len] = c;
			PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
				"Bogus field %.*s [%.*s].",
				key->len, key->s, arg->len, arg->s);
			goto done;
		}
		arg->s[arg->len] = c;
		break;
	default:
		PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
			"Unexpected type [%d] for field [%.*s]\n",
			val_type, key->len, key->s);
		goto done;
	}
	page->len = p - page->s;
	return 0;
done:
	page->len = p - page->s;
	return 1;
error:
	LM_ERR("buffer 2 small\n");
	page->len = p - page->s;
	return -1;
}



int ph_run_pi_cmd(int mod, int cmd,
			void *connection, void *con_cls,
			str *page, str *buffer)
{
	char *p;
	char *buf;
	int ret;

	str s_arg;
	str l_arg;
	str temp_holder;
	int temp_counter;
	int i;
	int j;
	int max_page_len;
	ph_cmd_t *command;

	int _len;
	int link_on;

	char tmp;
	char c[2];
	db_val_t *c_vals = NULL;
	db_val_t *q_vals = NULL;
	db_val_t *val;
	str val_str = {NULL, 0};
	int nr_rows;
	ph_db_url_t *db_url = NULL;
	db_res_t *res = NULL;
	db_val_t *values = NULL;
	db_row_t *rows;
	ph_mod_t *ph_modules;

	ph_modules = ph_framework_data->ph_modules;

	html_page_data.page.s = buffer->s;
	html_page_data.page.len = 0;
	html_page_data.buffer.s = buffer->s;
	html_page_data.buffer.len = buffer->len;
	html_page_data.mod = mod;
	html_page_data.cmd = cmd;
	max_page_len = buffer->len;

	if (0!=ph_build_header(page, buffer->len, mod, cmd)) return -1;
	buf = page->s;
	p = page->s + page->len;

	if (cmd<0) return ph_build_content(page, buffer->len, mod, cmd, NULL, NULL);

	httpd_api.lookup_arg(connection, "cmd", con_cls, &l_arg);
	if(l_arg.s==NULL) return ph_build_content(page, buffer->len, mod, cmd, NULL, NULL);

	LM_DBG("got arg cmd=[%.*s]\n", l_arg.len, l_arg.s);

	command = &ph_modules[mod].cmds[cmd];

	if (l_arg.len==3 && strncmp(l_arg.s, "pre", 3)==0) {
		/* We prebuild values only for update */
		if(command->type!=DB_CAP_UPDATE) {
			LM_ERR("command [%.*s] is not DB_CAP_UPDATE type\n",
				command->name.len, command->name.s);
			return ph_build_content(page, buffer->len, mod, cmd, NULL, NULL);
		}
		/* We prebuild values only for single clause update command */
		if(command->c_keys_size!=1) {
			LM_ERR("command [%.*s] has [%d] clause keys\n",
				command->name.len, command->name.s, command->c_keys_size);
			return ph_build_content(page, buffer->len, mod, cmd, NULL, NULL);
		}
		LM_DBG("[%.*s] with clause key [%.*s]\n",
			command->name.len, command->name.s,
			command->c_keys[0]->len, command->c_keys[0]->s);

		tmp = command->c_keys[0]->s[command->c_keys[0]->len];
		command->c_keys[0]->s[command->c_keys[0]->len] = '\0';
		LM_DBG("httpd_api.lookup_arg[%s]\n", command->c_keys[0]->s);
		httpd_api.lookup_arg(connection, command->c_keys[0]->s, con_cls, &l_arg);
		if(l_arg.s==NULL) {
			LM_ERR("missing clause key [%.*s] in args\n",
				command->c_keys[0]->len, command->c_keys[0]->s);
			command->c_keys[0]->s[command->c_keys[0]->len] = tmp;
			return ph_build_content(page, buffer->len, mod, cmd, NULL, NULL);
		}
		command->c_keys[0]->s[command->c_keys[0]->len] = tmp;

		LM_DBG("got clause [%.*s] with value [%.*s]\n",
			command->c_keys[0]->len, command->c_keys[0]->s, l_arg.len, l_arg.s);

		c_vals = (db_val_t*)pkg_malloc(command->c_keys_size*sizeof(db_val_t));
		if(c_vals==NULL){
			LM_ERR("oom\n");
			return -1;
		}
		memset(c_vals, 0, command->c_keys_size*sizeof(db_val_t));

		val = &c_vals[0];
		val->type = command->c_types[0];
		ret = getVal(val, command->c_types[0], command->c_keys[0],
				command->db_table, &l_arg, page, buffer, mod, cmd);
		if(ret<0)
			goto error;
		else if(ret>0)
			goto finish_page;

		/* Let's run the query to get the values for the record to update*/
		db_url = command->db_table->db_url;
		if(use_table(command->db_table)<0){
			PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
				"Error on table [%.*s].",
				command->db_table->name.len,
				command->db_table->name.s);
			goto finish_page;
		}

		if(db_url->http_dbf.query(*db_url->http_db_handle,
			command->c_keys, command->c_ops, c_vals,
			command->q_keys,
			command->c_keys_size,
			command->q_keys_size,
			command->o_keys?*command->o_keys:0, &res) < 0){
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
				"Error while querying database.");
			goto finish_page;
		}
		nr_rows = RES_ROW_N(res);
		switch (nr_rows) {
		case 0:
			LM_ERR("no record on clause key [%.*s]\n",
				command->c_keys[0]->len, command->c_keys[0]->s);
			if(c_vals) pkg_free(c_vals); c_vals = NULL;
			goto finish_page;
		case 1:
			LM_DBG("got [%d] rows for key [%.*s]\n",
				nr_rows, command->c_keys[0]->len, command->c_keys[0]->s);
			break;
		default:
			LM_ERR("to many records [%d] on clause key [%.*s]\n",
				nr_rows, command->c_keys[0]->len, command->c_keys[0]->s);
			goto finish_page;
		}

		rows = RES_ROWS(res);
		values = ROW_VALUES(rows);
		ret = ph_build_content(page, buffer->len, mod, cmd, &l_arg, values);
		db_url->http_dbf.free_result(*db_url->http_db_handle, res);
		//res = NULL;
		return ret;
	} else if(l_arg.len==2 && strncmp(l_arg.s, "on", 2)==0) {
		/* allocate c_vals array */
		if(command->c_keys_size && command->c_keys_size){
			c_vals = (db_val_t*)pkg_malloc(command->c_keys_size*sizeof(db_val_t));
			if(c_vals==NULL){
				LM_ERR("oom\n");
				return -1;
			}
			memset(c_vals, 0, command->c_keys_size*sizeof(db_val_t));
			for(i=0;i<command->c_keys_size;i++){
				s_arg.s = int2str(i, &s_arg.len);
				httpd_api.lookup_arg(connection, s_arg.s, con_cls, &l_arg);
				if(l_arg.s==NULL){
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"No argument for clause field #%d: %.*s.",
						i, command->c_keys[i]->len,
						command->c_keys[i]->s);
					goto done;
				}
				s_arg.len = l_arg.len;
				if(s_arg.len==0){
					PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
						"Empty argument for clause field #%d: %.*s.",
						i,command->c_keys[i]->len,
						command->c_keys[i]->s);
					goto done;
				}
				s_arg.s = l_arg.s;
				val = &c_vals[i];
				val->type = command->c_types[i];

				ret = getVal(val, command->c_types[i], command->c_keys[i],
					command->db_table, &s_arg, page, buffer, mod, cmd);
				if(ret<0)
					goto error;
				else if(ret>0)
					goto done;
			}
		}
	}
	if(command->q_keys_size && command->q_keys_size && command->type!=DB_CAP_QUERY){
		q_vals = (db_val_t*)pkg_malloc(command->q_keys_size*sizeof(db_val_t));
		if(q_vals==NULL){
			LM_ERR("oom\n");
			return -1;
		}
		memset(q_vals, 0, command->q_keys_size*sizeof(db_val_t));
		c[1] = '\0';
		for(i=0,c[0]='a';i<command->q_keys_size;i++,(c[0])++){
			if(c[0]=='z'){
				PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
					"Too many query values.");
				goto done;
			}
			LM_DBG("looking for arg [%s]\n", c);
			httpd_api.lookup_arg(connection, c, con_cls, &l_arg);
			if(l_arg.s==NULL){
				PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
					"No argument for query field #%d: %.*s.",
					i, command->q_keys[i]->len,
					command->q_keys[i]->s);
				goto done;
			}
			s_arg.len = l_arg.len;
			if(s_arg.len==0 && (command->q_types[i]!=DB_STR &&
					command->q_types[i]!=DB_STRING &&
					command->q_types[i]!=DB_BLOB)){
				PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
					"Empty argument for query field #%d: %.*s.",
					i, command->q_keys[i]->len,
					command->q_keys[i]->s);
				goto done;
			}
			s_arg.s = l_arg.s;
			val = &q_vals[i];
			val->type = command->q_types[i];
			ret = getVal(val, command->q_types[i], command->q_keys[i],
				command->db_table, &s_arg, page, buffer, mod, cmd);
			if(ret<0)
				goto error;
			else if(ret>0)
				goto done;

		}
	}

	db_url = command->db_table->db_url;
	if(use_table(command->db_table)<0){
		PI_HTTP_BUILD_REPLY(page, buffer, mod, cmd,
			"Error on table [%.*s].",
			command->db_table->name.len,
			command->db_table->name.s);
		goto done;
	}
	if(ph_build_reply(page, buffer->len, mod, cmd)<0)
		goto error;
	p = page->s + page->len;
	switch (command->type) {
	case DB_CAP_QUERY:
		for(j=0;j<command->q_keys_size;j++){
			if(j)PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_td_1d);
			PI_HTTP_COPY_2(p,*(command->q_keys[j]),
					PI_HTTP_Response_Menu_Cmd_td_4d);

		}
		if (DB_CAPABILITY(db_url->http_dbf, DB_CAP_FETCH)){
			if(db_url->http_dbf.query(*db_url->http_db_handle,
				command->c_keys, command->c_ops, c_vals,
				command->q_keys,
				command->c_keys_size,
				command->q_keys_size,
				command->o_keys?*command->o_keys:0, 0) < 0){
				PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Error while querying (fetch) database.");
				goto done;
			}
			if(db_url->http_dbf.fetch_result(*db_url->http_db_handle,
					&res, 100)<0){
				PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Fetching rows failed.");
				goto done;
			}
		}else{
			if(db_url->http_dbf.query(*db_url->http_db_handle,
				command->c_keys, command->c_ops, c_vals,
				command->q_keys,
				command->c_keys_size,
				command->q_keys_size,
				command->o_keys?*command->o_keys:0, &res) < 0){
				PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Error while querying database.");
				goto done;
			}
		}
		nr_rows = RES_ROW_N(res);
		do{
			LM_DBG("loading [%i] records from db\n", nr_rows);
			rows = RES_ROWS(res);
			for(i=0;i<nr_rows;i++){
				values = ROW_VALUES(rows + i);
				PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_tr_1);
				for(j=0;j<command->q_keys_size;j++){
					PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_td_1d);
					/* BEGIN */
					link_on = 0;
					if(command->link_cmd && command->link_cmd[j].s) {
						link_on = 1;
						PI_HTTP_COPY(p,PI_HTTP_HREF_1);
						if (http_root.len) {
							PI_HTTP_COPY_2(p,http_root, PI_HTTP_SLASH);
						}
						PI_HTTP_COPY_2(p,ph_modules[mod].module, PI_HTTP_SLASH);
						PI_HTTP_COPY(p,command->link_cmd[j]); /* this is the command */
						PI_HTTP_COPY_3(p,PI_HTTP_HREF_2,
								*command->q_keys[j],
								PI_HTTP_ATTR_VAL_SEPARATOR);
					}
					/* END */
					switch(command->q_types[j]){
					case DB_STR:
					case DB_STRING:
					case DB_BLOB:
						if(values[j].val.str_val.s==NULL){
							val_str.s = NULL; val_str.len = 0;
						} else {
							val_str.s = values[j].val.str_val.s;
							val_str.len = strlen(val_str.s);
						}
						LM_DBG("...got %.*s[%d]=>"
							"[%.*s][%.*s]\n",
							command->q_keys[j]->len,
							command->q_keys[j]->s, i,
							values[j].val.str_val.len,
							values[j].val.str_val.s,
							val_str.len, val_str.s);
						if (val_str.len) {
							if(link_on) {
								PI_HTTP_ESC_COPY(p, val_str, temp_holder, temp_counter);
								PI_HTTP_COPY(p,PI_HTTP_SQUOT_GT);
							}
							PI_HTTP_ESC_COPY(p, val_str, temp_holder, temp_counter);
						} else {
							if(link_on) {
								PI_HTTP_COPY(p, PI_HTTP_NBSP);
								PI_HTTP_COPY(p,PI_HTTP_SQUOT_GT);
							}
							PI_HTTP_COPY(p, PI_HTTP_NBSP);
						}
						break;
					case DB_INT:
						val_str.s = p;
						val_str.len = max_page_len - page->len;
						if(db_int2str(values[j].val.int_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert int [%d]\n",
								values[j].val.int_val);
							goto error;
						}
						p += val_str.len;
						page->len += val_str.len;
						if(link_on) PI_HTTP_COPY_2(p,PI_HTTP_SQUOT_GT,val_str);
						LM_DBG("   got %.*s[%d]=>"
							"[%d][%.*s]\n",
							command->q_keys[j]->len,
							command->q_keys[j]->s, i,
							values[j].val.int_val,
							val_str.len, val_str.s);
						break;
					case DB_BITMAP:
						val_str.s = p;
						val_str.len = max_page_len - page->len;
						if(db_int2str(values[j].val.bitmap_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert bitmap [%d]\n",
								values[j].val.bitmap_val);
							goto error;
						}
						p += val_str.len;
						page->len += val_str.len;
						if(link_on) PI_HTTP_COPY_2(p,PI_HTTP_SQUOT_GT,val_str);
						LM_DBG("   got %.*s[%d]=>"
							"[%d][%.*s]\n",
							command->q_keys[j]->len,
							command->q_keys[j]->s, i,
							values[j].val.bitmap_val,
							val_str.len, val_str.s);
						break;
					case DB_BIGINT:
						val_str.s = p;
						val_str.len = max_page_len - page->len;
						if(db_bigint2str(values[j].val.bigint_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert bigint [%-lld]\n",
								values[j].val.bigint_val);
							goto error;
						}
						p += val_str.len;
						page->len += val_str.len;
						if(link_on) PI_HTTP_COPY_2(p,PI_HTTP_SQUOT_GT,val_str);
						LM_DBG("   got %.*s[%d]=>"
							"[%-lld][%.*s]\n",
							command->q_keys[j]->len,
							command->q_keys[j]->s, i,
							values[j].val.bigint_val,
							val_str.len, val_str.s);
						break;
					case DB_DOUBLE:
						val_str.s = p;
						val_str.len = max_page_len - page->len;
						if(db_double2str(values[j].val.double_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert double [%-10.2f]\n",
								values[j].val.double_val);
							goto error;
						}
						p += val_str.len;
						page->len += val_str.len;
						if(link_on) PI_HTTP_COPY_2(p,PI_HTTP_SQUOT_GT,val_str);
						LM_DBG("   got %.*s[%d]=>"
							"[%-10.2f][%.*s]\n",
							command->q_keys[j]->len,
							command->q_keys[j]->s, i,
							values[j].val.double_val,
							val_str.len, val_str.s);
						break;
					case DB_DATETIME:
						val_str.s = p;
						val_str.len = max_page_len - page->len;
						if (db_time2str_nq(values[j].val.time_val,
									val_str.s, &val_str.len)!=0){
							LM_ERR("Unable to convert time [%ld]\n",
								(unsigned long int)values[j].val.time_val);
							goto error;
						}
						p += val_str.len;
						page->len += val_str.len;
						if(link_on) PI_HTTP_COPY_2(p,PI_HTTP_SQUOT_GT,val_str);
						LM_DBG("   got %.*s[%d]=>"
							"[%ld][%.*s]\n",
							command->q_keys[j]->len,
							command->q_keys[j]->s, i,
							(unsigned long int)values[j].val.time_val,
							val_str.len, val_str.s);
						break;
					default:
						LM_ERR("unexpected type [%d] "
							"for [%.*s]\n",
							command->q_types[j],
							command->q_keys[j]->len,
							command->q_keys[j]->s);
					}
					if(link_on) PI_HTTP_COPY(p,PI_HTTP_HREF_3);
					PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_td_4d);
				}
				PI_HTTP_COPY(p,PI_HTTP_Response_Menu_Cmd_tr_2);
			}
			/* any more data to be fetched ?*/
			if (DB_CAPABILITY(db_url->http_dbf, DB_CAP_FETCH)){
				if(db_url->http_dbf.fetch_result(*db_url->http_db_handle,
					&res, 100)<0){
					LM_ERR("fetching more rows failed\n");
					goto error;
				}
				nr_rows = RES_ROW_N(res);
			}else{
				nr_rows = 0;
			}
		}while (nr_rows>0);
		db_url->http_dbf.free_result(*db_url->http_db_handle, res);
		res=NULL;
		goto finish_page;
		break;
	case DB_CAP_INSERT:
		if((db_url->http_dbf.insert(*db_url->http_db_handle,
			command->q_keys, q_vals, command->q_keys_size))!=0){
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Unable to add record to db.");
		}else{
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Record successfully added to db.");
		}
		goto done;
		break;
	case DB_CAP_DELETE:
		if((db_url->http_dbf.delete(*db_url->http_db_handle,
			command->c_keys, command->c_ops, c_vals,
			command->c_keys_size))!=0) {
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Unable to delete record.");
		}else{
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Record successfully deleted from db.");
		}
		goto done;
		break;
	case DB_CAP_UPDATE:
		if((db_url->http_dbf.update(*db_url->http_db_handle,
			command->c_keys, command->c_ops, c_vals,
			command->q_keys, q_vals,
			command->c_keys_size, command->q_keys_size))!=0){
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Unable to update record.");
		}else{
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Record successfully updated.");
		}
		goto done;
		break;
	case DB_CAP_REPLACE:
		if((db_url->http_dbf.replace(*db_url->http_db_handle,
			command->q_keys, q_vals, command->q_keys_size))!=0){
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Unable to replace record.");
		}else{
			PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
					"Record successfully replaced.");
		}
		break;
	default:
		PI_HTTP_COMPLETE_REPLY(page, buffer, mod, cmd,
			"Corrupt data for mod=[%d] and cmd=[%d]\n", mod, cmd);
		goto done;
	}
	LM_ERR("You shoudn't end up here\n");
error:
	if (db_url && res)
		db_url->http_dbf.free_result(*db_url->http_db_handle, res);
	if(c_vals) pkg_free(c_vals);
	if(q_vals) pkg_free(q_vals);
	return -1;

finish_page:
	if(c_vals) pkg_free(c_vals);
	if(q_vals) pkg_free(q_vals);
	page->len = p - page->s;
	return ph_build_reply_footer(page, buffer->len);

done:
	if(c_vals) pkg_free(c_vals);
	if(q_vals) pkg_free(q_vals);
	return 0;
}

