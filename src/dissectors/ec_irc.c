/*
    ettercap -- dissector irc -- TCP 6666 6667 6668 6669

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ec_irc.c,v 1.2 2003/08/04 13:59:07 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>
#include <ec_strings.h>

/* protos */

FUNC_DECODER(dissector_irc);
void irc_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init irc_init(void)
{
   dissect_add("irc", APP_LAYER_TCP, 6666, dissector_irc);
   dissect_add("irc", APP_LAYER_TCP, 6667, dissector_irc);
   dissect_add("irc", APP_LAYER_TCP, 6668, dissector_irc);
   dissect_add("irc", APP_LAYER_TCP, 6669, dissector_irc);
}

FUNC_DECODER(dissector_irc)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct session *s = NULL;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* unused variable */
   (void)end;

   /* skip messages coming from the server */
   if (dissect_on_port("irc", ntohs(PACKET->L4.src)) == ESUCCESS)
      return NULL;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   DEBUG_MSG("IRC --> TCP dissector_irc");
 
/*
 * authentication method: PASS
 *
 * /PASS password
 * 
 */
   if ( !strncasecmp(ptr, "PASS ", 5) ) {

      DEBUG_MSG("\tDissector_irc PASS");
      
      ptr += 5;

      dissect_create_ident(&ident, PACKET);
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
     
      SAFE_FREE(ident);
      
      PACKET->DISSECTOR.pass = strdup(ptr);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/PASS password");
      
      USER_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }
   
/*
 * changing a channel key
 *
 * /MODE #channel +k password
 * 
 */
   if ( !strncasecmp(ptr, "MODE ", 5) && match_pattern(ptr + 5, "#* +k *") ) {

      DEBUG_MSG("\tDissector_irc MODE");
      
      ptr += 5;
      
      /* fill the structure */
      PACKET->DISSECTOR.user = strdup(ptr);
      if ( (ptr = strchr(PACKET->DISSECTOR.user, ' ')) != NULL )
         *ptr = '\0';
     
      /* skip the " +k " */
      PACKET->DISSECTOR.pass = strdup(ptr + 4);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/MODE #channel +k password");
      
      USER_MSG("IRC : %s:%d -> CHANNEL: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * entering in a channel with a key
 *
 * /JOIN #channel password
 * 
 */
   if ( !strncasecmp(ptr, "JOIN ", 5) && match_pattern(ptr + 5, "#* *") ) {

      DEBUG_MSG("\tDissector_irc JOIN");
      
      ptr += 5;
      
      /* fill the structure */
      PACKET->DISSECTOR.user = strdup(ptr);
      if ( (ptr = strchr(PACKET->DISSECTOR.user, ' ')) != NULL )
         *ptr = '\0';
     
      PACKET->DISSECTOR.pass = strdup(ptr + 1);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/JOIN #channel password");
      
      USER_MSG("IRC : %s:%d -> CHANNEL: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * identifying to the nickserv
 *
 * /msg nickserv identify password
 * 
 */
   if ( !strncasecmp(ptr, "PRIVMSG ", 8) && match_pattern(ptr + 8, "* :identify *\r\n") ) {
      char *pass;

      DEBUG_MSG("\tDissector_irc PRIVMSG");
      
      if (!(pass = strcasestr(ptr, "identify")))
         return NULL;
      
      pass += 9;

      dissect_create_ident(&ident, PACKET);
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
     
      SAFE_FREE(ident);
     
      PACKET->DISSECTOR.pass = strdup(pass);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/msg nickserv identify password");
      
      USER_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }
   
/*
 * identifying to the nickserv
 *
 * /nickserv identify password
 * 
 */
   if ( !strncasecmp(ptr, "NICKSERV ", 9) || !strncasecmp(ptr, "NS ", 3) ) {
      char *pass;

      DEBUG_MSG("\tDissector_irc NICKSERV");
      
      if (!(pass = strcasestr(ptr, "identify")))
         return NULL;
      
      pass += 9;

      dissect_create_ident(&ident, PACKET);
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
     
      SAFE_FREE(ident);
     
      PACKET->DISSECTOR.pass = strdup(pass);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/nickserv identify password");
      
      USER_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * identifying to the nickserv
 *
 * /identify password
 * 
 */
   if ( !strncasecmp(ptr, "IDENTIFY ", 9) ) {
      char *pass;

      DEBUG_MSG("\tDissector_irc IDENTIFY");
      
      if (!(pass = strcasestr(ptr, " ")))
         return NULL;
      
      /* adjust the pointer */
      if (*++pass == ':') 
         pass += 1;

      dissect_create_ident(&ident, PACKET);
      
      /* get the saved nick */
      if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS)
         PACKET->DISSECTOR.user = strdup(s->data);
      else
         PACKET->DISSECTOR.user = strdup("unknown");
     
      SAFE_FREE(ident);
     
      PACKET->DISSECTOR.pass = strdup(pass);
      if ( (ptr = strchr(PACKET->DISSECTOR.pass, '\r')) != NULL )
         *ptr = '\0';

      PACKET->DISSECTOR.info = strdup("/identify password");
      
      USER_MSG("IRC : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    PACKET->DISSECTOR.user,
                                    PACKET->DISSECTOR.pass,
                                    PACKET->DISSECTOR.info);
      
      return NULL;
   }

/*
 * register the nick in the session
 * list, we need it later when printing 
 * passwords.
 */

   /* user is taking a nick */
   if (!strncasecmp(ptr, "NICK ", 5)) {
      char *p;
      char *user;
      ptr += 5;
      
      /* delete any previous saved nick */
      dissect_wipe_session(PACKET);
      /* create the new session */
      dissect_create_session(&s, PACKET);
     
      /* save the nick */
      s->data = strdup(ptr);
      if ( (p = strchr(s->data, '\r')) != NULL )
         *p = '\0';

      /* print the user info */
      if ((ptr = strcasestr(ptr, "USER "))) {
         user = strdup(ptr + 5);
         if ( (p = strchr(user, '\r')) != NULL )
            *p = '\0';
         
         USER_MSG("IRC : %s:%d -> USER: %s (%s)\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                    ntohs(PACKET->L4.dst), 
                                    s->data, 
                                    user);
         SAFE_FREE(user);
      }

      /* save the session */
      session_put(s);

      return NULL;
   }

   /* delete the user */
   if (!strncasecmp(ptr, "QUIT ", 5)) {
      dissect_wipe_session(PACKET);

      return NULL;
   }
   
   return NULL;
}


/* EOF */

// vim:ts=3:expandtab

