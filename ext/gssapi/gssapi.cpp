/* Copyright (c) 2010, Jean-Yves Avenard and Hydrix Pty Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Hydrix nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include <windows.h>
#include <stdio.h>

#include "gssapi.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Reserved static storage for GSS_oids.  Comments are quotes from RFC 2744. */
#define oids ((gss_OID_desc *)const_oids)
static const gss_OID_desc const_oids[] = {
    /*
     * The implementation must reserve static storage for a
	 * gss_OID_desc object containing the value */
    {10, (void *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x01"},
    /* corresponding to an object-identifier value of
	 * {iso(1) member-body(2) United States(840) mit(113554)
	 * infosys(1) gssapi(2) generic(1) user_name(1)}.  The constant
	 * GSS_C_NT_USER_NAME should be initialized to point
	 * to that gss_OID_desc.
	 */

    /*
	 * The implementation must reserve static storage for a
	 * gss_OID_desc object containing the value */
    {10, (void *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x02"},
    /* corresponding to an object-identifier value of
	 * {iso(1) member-body(2) United States(840) mit(113554)
	 * infosys(1) gssapi(2) generic(1) machine_uid_name(2)}.
	 * The constant GSS_C_NT_MACHINE_UID_NAME should be
	 * initialized to point to that gss_OID_desc.
	 */

    /*
    * The implementation must reserve static storage for a
    * gss_OID_desc object containing the value */
    {10, (void *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x03"},
    /* corresponding to an object-identifier value of
    * {iso(1) member-body(2) United States(840) mit(113554)
    * infosys(1) gssapi(2) generic(1) string_uid_name(3)}.
    * The constant GSS_C_NT_STRING_UID_NAME should be
    * initialized to point to that gss_OID_desc.
    */

    /*
     * The implementation must reserve static storage for a
     * gss_OID_desc object containing the value */
    {6, (void *)"\x2b\x06\x01\x05\x06\x02"},
    /* corresponding to an object-identifier value of
     * {iso(1) org(3) dod(6) internet(1) security(5)
     * nametypes(6) gss-host-based-services(2)).  The constant
     * GSS_C_NT_HOSTBASED_SERVICE_X should be initialized to point
     * to that gss_OID_desc.  This is a deprecated OID value, and
     * implementations wishing to support hostbased-service names
     * should instead use the GSS_C_NT_HOSTBASED_SERVICE OID,
     * defined below, to identify such names;
     * GSS_C_NT_HOSTBASED_SERVICE_X should be accepted a synonym
     * for GSS_C_NT_HOSTBASED_SERVICE when presented as an input
     * parameter, but should not be emitted by GSS-API
     * implementations
     */

    /*
     * The implementation must reserve static storage for a
     * gss_OID_desc object containing the value */
    {10, (void *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x04"},
    /* corresponding to an object-identifier value of
     * {iso(1) member-body(2) Unites States(840) mit(113554)
     * infosys(1) gssapi(2) generic(1) service_name(4)}.
     * The constant GSS_C_NT_HOSTBASED_SERVICE should be
     * initialized to point to that gss_OID_desc.
     */

    /*
     * The implementation must reserve static storage for a
     * gss_OID_desc object containing the value */
    {6, (void *)"\x2b\x06\01\x05\x06\x03"},
    /* corresponding to an object identifier value of
     * {1(iso), 3(org), 6(dod), 1(internet), 5(security),
     * 6(nametypes), 3(gss-anonymous-name)}.  The constant
     * and GSS_C_NT_ANONYMOUS should be initialized to point
     * to that gss_OID_desc.
     */

    /*
     * The implementation must reserve static storage for a
     * gss_OID_desc object containing the value */
    {6, (void *)"\x2b\x06\x01\x05\x06\x04"},
    /* corresponding to an object-identifier value of
     * {1(iso), 3(org), 6(dod), 1(internet), 5(security),
     * 6(nametypes), 4(gss-api-exported-name)}.  The constant
     * GSS_C_NT_EXPORT_NAME should be initialized to point
     * to that gss_OID_desc.
     */
};

/* Here are the constants which point to the static structure above.
 *
 * Constants of the form GSS_C_NT_* are specified by rfc 2744.
 *
 * Constants of the form gss_nt_* are the original MIT krb5 names
 * found in gssapi_generic.h.  They are provided for compatibility. */

gss_OID GSS_C_NT_USER_NAME           = oids+0;
gss_OID gss_nt_user_name             = oids+0;

gss_OID GSS_C_NT_MACHINE_UID_NAME    = oids+1;
gss_OID gss_nt_machine_uid_name      = oids+1;

gss_OID GSS_C_NT_STRING_UID_NAME     = oids+2;
gss_OID gss_nt_string_uid_name       = oids+2;

gss_OID GSS_C_NT_HOSTBASED_SERVICE_X = oids+3;
gss_OID gss_nt_service_name_v2       = oids+3;

gss_OID GSS_C_NT_HOSTBASED_SERVICE   = oids+4;
gss_OID gss_nt_service_name          = oids+4;

gss_OID GSS_C_NT_ANONYMOUS           = oids+5;

gss_OID GSS_C_NT_EXPORT_NAME         = oids+6;
gss_OID gss_nt_exported_name         = oids+6;

#ifdef WIN64
#define GSSAPI_DLL "gssapi64.dll"
#else
#define GSSAPI_DLL "gssapi32.dll"
#endif

static HINSTANCE hinstLib = LoadLibrary(TEXT(GSSAPI_DLL)); // Will let Windows automatically call FreeLibrary. This is for speed-sake

#define CALL_ENTRY(x, type, ...) { \
    if (hinstLib != NULL) \
    { \
        ProcAdd = (type)(GetProcAddress(hinstLib, TEXT(x))); \
        if (NULL != ProcAdd) \
        { \
            return (ProcAdd) (__VA_ARGS__); \
        } \
    } \
    *a1 = 0; \
    return GSS_S_FAILURE; \
}

OM_uint32 gss_display_status
(OM_uint32 *a1,		/* minor_status */
            OM_uint32 a2,			/* status_value */
            int a3,			/* status_type */
            gss_OID a4,			/* mech_type (used to be const) */
            OM_uint32 *a5,		/* message_context */
            gss_buffer_t a6		/* status_string */
           )
{
    typedef OM_uint32 (WINAPI *pgss_display_status)(OM_uint32 *, OM_uint32 , int, gss_OID, OM_uint32 *, gss_buffer_t );
    pgss_display_status ProcAdd;

    CALL_ENTRY("gss_display_status", pgss_display_status, a1, a2, a3, a4, a5, a6);
}

OM_uint32 gss_release_buffer
(OM_uint32 *a1,		/* minor_status */
            gss_buffer_t a2		/* buffer */
           )
{
    typedef OM_uint32 (WINAPI *pgss_release_buffer)(OM_uint32 *, gss_buffer_t);
    pgss_release_buffer ProcAdd;

    CALL_ENTRY("gss_release_buffer", pgss_release_buffer, a1, a2);
}

OM_uint32 gss_wrap
(OM_uint32 *a1,		/* minor_status */
	    gss_ctx_id_t a2,		/* context_handle */
	    int a3,			/* conf_req_flag */
	    gss_qop_t a4,			/* qop_req */
	    gss_buffer_t a5,		/* input_message_buffer */
	    int *a6,			/* conf_state */
	    gss_buffer_t a7		/* output_message_buffer */
	   )
{
    typedef OM_uint32 (WINAPI *pgss_wrap)(OM_uint32 *, gss_ctx_id_t, int, gss_qop_t, gss_buffer_t, int *, gss_buffer_t);
    pgss_wrap ProcAdd;

    CALL_ENTRY("gss_wrap", pgss_wrap, a1, a2, a3, a4, a5, a6, a7);
}

OM_uint32 gss_unwrap
(OM_uint32 *a1,		/* minor_status */
	    gss_ctx_id_t a2,		/* context_handle */
	    gss_buffer_t a3,		/* input_message_buffer */
	    gss_buffer_t a4,		/* output_message_buffer */
	    int * a5,			/* conf_state */
	    gss_qop_t *a6		/* qop_state */
	   )
{
    typedef OM_uint32 (WINAPI *pgss_unwrap)(OM_uint32 *, gss_ctx_id_t , gss_buffer_t, gss_buffer_t, int *, gss_qop_t *);
    pgss_unwrap ProcAdd;

    CALL_ENTRY("gss_unwrap", pgss_unwrap, a1, a2, a3, a4, a5, a6);
}

OM_uint32 gss_delete_sec_context
(OM_uint32 *a1,		/* minor_status */
            gss_ctx_id_t *a2,		/* context_handle */
            gss_buffer_t a3		/* output_token */
           )
{
    typedef OM_uint32 (WINAPI *pgss_delete_sec_context)(OM_uint32 *, gss_ctx_id_t *, gss_buffer_t);
    pgss_delete_sec_context ProcAdd;

    CALL_ENTRY("gss_delete_sec_context", pgss_delete_sec_context, a1, a2, a3);
}

OM_uint32 gss_release_name
(OM_uint32 *a1,		/* minor_status */
            gss_name_t *a2		/* input_name */
           )
{
    typedef OM_uint32 (WINAPI *pgss_release_name)(OM_uint32 *, gss_name_t *);
    pgss_release_name ProcAdd;

    CALL_ENTRY("gss_release_name", pgss_release_name, a1, a2);
}

OM_uint32 gss_release_cred
(OM_uint32 *a1,		/* minor_status */
            gss_cred_id_t *a2		/* cred_handle */
           )
{
    typedef OM_uint32 (WINAPI *pgss_release_cred)(OM_uint32 *, gss_cred_id_t *);
    pgss_release_cred ProcAdd;

    CALL_ENTRY("gss_release_cred", pgss_release_cred, a1, a2);
}

OM_uint32 gss_import_name
(OM_uint32 *a1,		/* minor_status */
            gss_buffer_t a2,		/* input_name_buffer */
            gss_OID a3,			/* input_name_type(used to be const) */
            gss_name_t *a4		/* output_name */
           )
{
    typedef OM_uint32 (WINAPI *pgss_import_name)(OM_uint32 *, gss_buffer_t , gss_OID, gss_name_t *);
    pgss_import_name ProcAdd;

    CALL_ENTRY("gss_import_name", pgss_import_name, a1, a2, a3, a4);
}

OM_uint32 gss_acquire_cred
(OM_uint32 *a1,		/* minor_status */
            gss_name_t a2,			/* desired_name */
            OM_uint32 a3,			/* time_req */
            gss_OID_set a4,		/* desired_mechs */
            gss_cred_usage_t a5,		/* cred_usage */
            gss_cred_id_t *a6,	/* output_cred_handle */
            gss_OID_set *a7,		/* actual_mechs */
            OM_uint32 *a8		/* time_rec */
           )
{
    typedef OM_uint32 (WINAPI *pgss_acquire_cred)(OM_uint32 *, gss_name_t , OM_uint32, gss_OID_set, gss_cred_usage_t, gss_cred_id_t *, gss_OID_set *, OM_uint32 *);
    pgss_acquire_cred ProcAdd;

    CALL_ENTRY("gss_acquire_cred", pgss_acquire_cred, a1, a2, a3, a4, a5, a6, a7, a8);
}

OM_uint32 gss_accept_sec_context
(OM_uint32 *a1,		/* minor_status */
            gss_ctx_id_t *a2,		/* context_handle */
            gss_cred_id_t a3,		/* acceptor_cred_handle */
            gss_buffer_t a4,		/* input_token_buffer */
            gss_channel_bindings_t a5,	/* input_chan_bindings */
            gss_name_t *a6,		/* src_name */
            gss_OID *a7,		/* mech_type */
            gss_buffer_t a8,		/* output_token */
            OM_uint32 *a9,		/* ret_flags */
            OM_uint32 *a10,		/* time_rec */
            gss_cred_id_t *a11		/* delegated_cred_handle */
           )
{
    typedef OM_uint32 (WINAPI *pgss_accept_sec_context)(OM_uint32 *, gss_ctx_id_t *, gss_cred_id_t, gss_buffer_t, gss_channel_bindings_t, gss_name_t *, gss_OID *, gss_buffer_t, OM_uint32 *, OM_uint32 *, gss_cred_id_t *);
    pgss_accept_sec_context ProcAdd;

    CALL_ENTRY("gss_accept_sec_context", pgss_accept_sec_context, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

OM_uint32 gss_compare_name
(OM_uint32 *a1,		/* minor_status */
            gss_name_t a2,			/* name1 */
            gss_name_t a3,			/* name2 */
            int *a4			/* name_equal */
           )
{
    typedef OM_uint32 (WINAPI *pgss_compare_name)(OM_uint32 *, gss_name_t , gss_name_t, int *);
    pgss_compare_name ProcAdd;

    CALL_ENTRY("gss_compare_name", pgss_compare_name, a1, a2, a3, a4);
}

OM_uint32 gss_wrap_size_limit
(OM_uint32 *a1,		/* minor_status */
	    gss_ctx_id_t a2,		/* context_handle */
	    int a3,			/* conf_req_flag */
	    gss_qop_t a4,			/* qop_req */
	    OM_uint32 a5,			/* req_output_size */
	    OM_uint32 *a6			/* max_input_size */
	   )
{
    typedef OM_uint32 (WINAPI *pgss_wrap_size_limit)(OM_uint32 *, gss_ctx_id_t, int, gss_qop_t, OM_uint32, OM_uint32 *);
    pgss_wrap_size_limit ProcAdd;

    CALL_ENTRY("gss_wrap_size_limit", pgss_wrap_size_limit, a1, a2, a3, a4, a5, a6);
}

OM_uint32 gss_init_sec_context
(OM_uint32 *a1,		/* minor_status */
            gss_cred_id_t a2,		/* claimant_cred_handle */
            gss_ctx_id_t *a3,		/* context_handle */
            gss_name_t a4,			/* target_name */
            gss_OID a5,			/* mech_type (used to be const) */
            OM_uint32 a6,			/* req_flags */
            OM_uint32 a7,			/* time_req */
            gss_channel_bindings_t a8,	/* input_chan_bindings */
            gss_buffer_t a9,		/* input_token */
            gss_OID *a10,		/* actual_mech_type */
            gss_buffer_t a11,		/* output_token */
            OM_uint32 *a12,		/* ret_flags */
            OM_uint32 *a13		/* time_rec */
           )
{
    typedef OM_uint32 (WINAPI *pgss_init_sec_context)(OM_uint32 *, gss_cred_id_t, gss_ctx_id_t *, gss_name_t, gss_OID, OM_uint32, OM_uint32, gss_channel_bindings_t, gss_buffer_t, gss_OID *, gss_buffer_t, OM_uint32 *, OM_uint32 *);
    pgss_init_sec_context ProcAdd;

    CALL_ENTRY("gss_init_sec_context", pgss_init_sec_context, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
}

OM_uint32 gss_inquire_context
(OM_uint32 *a1,		/* minor_status */
	    gss_ctx_id_t a2,		/* context_handle */
	    gss_name_t *a3,		/* src_name */
	    gss_name_t *a4,		/* targ_name */
	    OM_uint32 *a5,		/* lifetime_rec */
	    gss_OID *a6,		/* mech_type */
	    OM_uint32 *a7,		/* ctx_flags */
	    int *a8,           	/* locally_initiated */
	    int *a9			/* open */
	   )
{
    typedef OM_uint32 (WINAPI *pgss_inquire_context)(OM_uint32 *, gss_ctx_id_t, gss_name_t *, gss_name_t *, OM_uint32 *, gss_OID *, OM_uint32 *, int *, int *);
    pgss_inquire_context ProcAdd;

    CALL_ENTRY("gss_inquire_context", pgss_inquire_context, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

OM_uint32 gss_display_name
(OM_uint32 *a1,		/* minor_status */
            gss_name_t a2,			/* input_name */
            gss_buffer_t a3,		/* output_name_buffer */
            gss_OID *a4		/* output_name_type */
           )
{
    typedef OM_uint32 (WINAPI *pgss_display_name)(OM_uint32 *, gss_name_t, gss_buffer_t, gss_OID *);
    pgss_display_name ProcAdd;

    CALL_ENTRY("gss_display_name", pgss_display_name, a1, a2, a3, a4);
}

#ifdef __cplusplus
}
#endif
