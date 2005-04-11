
/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */

/* (C) COPYRIGHT International Business Machines Corp. 2001, 2002, 2005 */


// File:  decr_mgr.c
//
// Decryption manager routines
//

//#include <windows.h>

#include <pthread.h>
  #include <string.h>            // for memcmp() et al
  #include <stdlib.h>

#include "pkcs11/pkcs11types.h"
#include <pkcs11/stdll.h>
#include "defs.h"
#include "host_defs.h"
#include "h_extern.h"
#include "tok_spec_struct.h"
//#include "args.h"


//
//
CK_RV
decr_mgr_init( SESSION           *sess,
               ENCR_DECR_CONTEXT *ctx,
               CK_ULONG           operation,
               CK_MECHANISM      *mech,
               CK_OBJECT_HANDLE   key_handle )
{
   OBJECT        * key_obj = NULL;
   CK_ATTRIBUTE  * attr    = NULL;
   CK_BYTE     * ptr     = NULL;
   CK_KEY_TYPE   keytype;
   CK_BBOOL      flag;
   CK_RV         rc;



   if (!sess){
      st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
      return CKR_FUNCTION_FAILED;
   }
   if (ctx->active != FALSE){
      st_err_log(31, __FILE__, __LINE__);
      return CKR_OPERATION_ACTIVE;
   }

   // key usage restrictions
   //
   if (operation == OP_DECRYPT_INIT)
   {
      rc = object_mgr_find_in_map1( key_handle, &key_obj );
      if (rc != CKR_OK){
         st_err_log(18, __FILE__, __LINE__);
         return CKR_KEY_HANDLE_INVALID;
      }
      // is key allowed to do general decryption?
      //
      rc = template_attribute_find( key_obj->template, CKA_DECRYPT, &attr );
      if (rc == FALSE){
         st_err_log(85, __FILE__, __LINE__);
         return CKR_KEY_FUNCTION_NOT_PERMITTED;
      }
      else
      {
         flag = *(CK_BBOOL *)attr->pValue;
         if (flag != TRUE){
            st_err_log(85, __FILE__, __LINE__);
            return CKR_KEY_FUNCTION_NOT_PERMITTED;
         }
      }
   }
   else if (operation == OP_UNWRAP)
   {
      rc = object_mgr_find_in_map1( key_handle, &key_obj );
      if (rc != CKR_OK){
         st_err_log(62, __FILE__, __LINE__);
         return CKR_WRAPPING_KEY_HANDLE_INVALID;
      }
      // is key allowed to unwrap other keys?
      //
      rc = template_attribute_find( key_obj->template, CKA_UNWRAP, &attr );
      if (rc == FALSE){
         st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
         return CKR_FUNCTION_FAILED; // Cryptoki doesn't define a better return code
      }
      else
      {
         flag = *(CK_BBOOL *)attr->pValue;
         if (flag == FALSE){
            st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
            return CKR_FUNCTION_FAILED;
         }
      }
   }
   else{
      st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
      return CKR_FUNCTION_FAILED;
   }
   // is the mechanism supported?  is the key type correct?  is a
   // parameter present if required?  is the key size allowed?
   // does the key support decryption?
   //
   // Will the FCV allow the operation?
   //
   switch (mech->mechanism)
   {
      case CKM_DES_ECB:
         {
            if (mech->ulParameterLen != 0){
               st_err_log(29, __FILE__, __LINE__);
               return CKR_MECHANISM_PARAM_INVALID;
            }
            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_DES){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            // Check FCV
            //
//            if ((nv_FCV.FunctionCntlBytes[DES_FUNCTION_BYTE] & FCV_56_BIT_DES) == 0)
//               return CKR_MECHANISM_INVALID;

            ctx->context_len = sizeof(DES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(DES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(DES_CONTEXT) );
         }
         break;

      case CKM_CDMF_ECB:
         {
            if (mech->ulParameterLen != 0){
               st_err_log(29, __FILE__, __LINE__);
               return CKR_MECHANISM_PARAM_INVALID;
            }
            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_CDMF){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            // Check FCV
            //
//            if ((nv_FCV.FunctionCntlBytes[DES_FUNCTION_BYTE] & FCV_CDMF_DES) == 0)
//               return CKR_MECHANISM_INVALID;

            ctx->context_len = sizeof(DES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(DES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(DES_CONTEXT) );
         }
         break;

      case CKM_DES_CBC:
      case CKM_DES_CBC_PAD:
         {
            if (mech->ulParameterLen != DES_BLOCK_SIZE){
               st_err_log(29, __FILE__, __LINE__);
               return CKR_MECHANISM_PARAM_INVALID;
            }
            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_DES){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            // Check FCV
            //
//            if ((nv_FCV.FunctionCntlBytes[DES_FUNCTION_BYTE] & FCV_56_BIT_DES) == 0)
//               return CKR_MECHANISM_INVALID;

            ctx->context_len = sizeof(DES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(DES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(DES_CONTEXT) );
         }
         break;

      case CKM_CDMF_CBC:
      case CKM_CDMF_CBC_PAD:
         {
            if (mech->ulParameterLen != DES_BLOCK_SIZE){
               st_err_log(29, __FILE__, __LINE__);
               return CKR_MECHANISM_PARAM_INVALID;
            }
            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_CDMF){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }


            ctx->context_len = sizeof(DES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(DES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(DES_CONTEXT) );
         }
         break;

      case CKM_DES3_ECB:
         {
            if (mech->ulParameterLen != 0)
               return CKR_MECHANISM_PARAM_INVALID;


            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_DES3 && keytype != CKK_DES2){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            // Check FCV
            //
//            if ((nv_FCV.FunctionCntlBytes[DES_FUNCTION_BYTE] & FCV_TRIPLE_DES) == 0)
//               return CKR_MECHANISM_INVALID;

            ctx->context_len = sizeof(DES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(DES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(DES_CONTEXT) );
         }
         break;

      case CKM_DES3_CBC:
      case CKM_DES3_CBC_PAD:
         {
            if (mech->ulParameterLen != DES_BLOCK_SIZE)
               return CKR_MECHANISM_PARAM_INVALID;

            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_DES3 && keytype != CKK_DES2){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            // Check FCV
            //
//            if ((nv_FCV.FunctionCntlBytes[DES_FUNCTION_BYTE] & FCV_TRIPLE_DES) == 0)
//               return CKR_MECHANISM_INVALID;

            ctx->context_len = sizeof(DES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(DES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(DES_CONTEXT) );
         }
         break;

      case CKM_RSA_X_509:
      case CKM_RSA_PKCS:
         {
            if (mech->ulParameterLen != 0)
               return CKR_MECHANISM_PARAM_INVALID;

            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_RSA){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            // Check FCV
            //
//            rc = template_attribute_find( key_obj->template, CKA_MODULUS, &attr );
//            if (rc == FALSE ||
//                nv_FCV.SymmetricModLength/8 < attr->value_length)
//               return (operation == OP_DECRYPT_INIT ? CKR_KEY_SIZE_RANGE : CKR_UNWRAPPING_KEY_SIZE_RANGE );

            // RSA cannot be used for multi-part operations
            //
            ctx->context_len = 0;
            ctx->context     = NULL;

         }
         break;
      case CKM_AES_ECB:
	 {
	    // XXX Copied from DES3, should be verified - KEY
		 
            if (mech->ulParameterLen != 0)
               return CKR_MECHANISM_PARAM_INVALID;


            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_AES){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            ctx->context_len = sizeof(AES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(AES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(AES_CONTEXT) );

	 }
	 break;
      case CKM_AES_CBC:
      case CKM_AES_CBC_PAD:
	 {
	    // XXX Copied from DES3, should be verified - KEY
		 
            if (mech->ulParameterLen != AES_INIT_VECTOR_SIZE)
               return CKR_MECHANISM_PARAM_INVALID;

            // is the key type correct?
            //
            rc = template_attribute_find( key_obj->template, CKA_KEY_TYPE, &attr );
            if (rc == FALSE){
               st_err_log(20, __FILE__, __LINE__);
               return CKR_KEY_TYPE_INCONSISTENT;
            }
            else
            {
               keytype = *(CK_KEY_TYPE *)attr->pValue;
               if (keytype != CKK_AES){
                  st_err_log(20, __FILE__, __LINE__);
                  return CKR_KEY_TYPE_INCONSISTENT;
               }
            }

            ctx->context_len = sizeof(AES_CONTEXT);
            ctx->context     = (CK_BYTE *)malloc(sizeof(AES_CONTEXT));
            if (!ctx->context){
               st_err_log(1, __FILE__, __LINE__);
               return CKR_HOST_MEMORY;
            }
            memset( ctx->context, 0x0, sizeof(AES_CONTEXT) );

	 }
	 break;
	 
      default:
         st_err_log(28, __FILE__, __LINE__);
         return CKR_MECHANISM_INVALID;
   }


   if (mech->ulParameterLen > 0) {
      ptr = (CK_BYTE *)malloc(mech->ulParameterLen);
      if (!ptr){
         st_err_log(1, __FILE__, __LINE__);
         return CKR_HOST_MEMORY;
      }
      memcpy( ptr, mech->pParameter, mech->ulParameterLen );
   }

   ctx->key                 = key_handle;
   ctx->mech.ulParameterLen = mech->ulParameterLen;
   ctx->mech.mechanism      = mech->mechanism;
   ctx->mech.pParameter     = ptr;
   ctx->multi               = FALSE;
   ctx->active              = TRUE;

   return CKR_OK;
}


//
//
CK_RV
decr_mgr_cleanup( ENCR_DECR_CONTEXT *ctx )
{
   if (!ctx){
      st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
      return CKR_FUNCTION_FAILED;
   }
   ctx->key                 = 0;
   ctx->mech.ulParameterLen = 0;
   ctx->mech.mechanism      = 0;
   ctx->multi               = FALSE;
   ctx->active              = FALSE;
   ctx->context_len         = 0;

   if (ctx->mech.pParameter) {
      free( ctx->mech.pParameter );
      ctx->mech.pParameter = NULL;
   }

   if (ctx->context) {
      free( ctx->context );
      ctx->context = NULL;
   }

   return CKR_OK;
}


//
//
CK_RV
decr_mgr_decrypt( SESSION           *sess,
                  CK_BBOOL           length_only,
                  ENCR_DECR_CONTEXT *ctx,
                  CK_BYTE           *in_data,
                  CK_ULONG           in_data_len,
                  CK_BYTE           *out_data,
                  CK_ULONG          *out_data_len )
{
   if (!sess || !ctx){
      st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
      return CKR_FUNCTION_FAILED;
   }
   if (ctx->active == FALSE){
      st_err_log(32, __FILE__, __LINE__);
      return CKR_OPERATION_NOT_INITIALIZED;
   }
   // if the caller just wants the decrypted length, there is no reason to
   // specify the input data.  I just need the data length
   //
   if ((length_only == FALSE) && (!in_data || !out_data)){
      st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
      return CKR_FUNCTION_FAILED;
   }
   if (ctx->multi == TRUE){
      st_err_log(31, __FILE__, __LINE__);
      return CKR_OPERATION_ACTIVE;
   }
   switch (ctx->mech.mechanism) {
      case CKM_CDMF_ECB:
      case CKM_DES_ECB:
         return des_ecb_decrypt( sess,     length_only,
                                 ctx,
                                 in_data,  in_data_len,
                                 out_data, out_data_len );

      case CKM_CDMF_CBC:
      case CKM_DES_CBC:
         return des_cbc_decrypt( sess,     length_only,
                                 ctx,
                                 in_data,  in_data_len,
                                 out_data, out_data_len );

      case CKM_DES_CBC_PAD:
      case CKM_CDMF_CBC_PAD:
         return des_cbc_pad_decrypt( sess,     length_only,
                                     ctx,
                                     in_data,  in_data_len,
                                     out_data, out_data_len );

      case CKM_DES3_ECB:
         return des3_ecb_decrypt( sess,     length_only,
                                  ctx,
                                  in_data,  in_data_len,
                                  out_data, out_data_len );

      case CKM_DES3_CBC:
         return des3_cbc_decrypt( sess,     length_only,
                                  ctx,
                                  in_data,  in_data_len,
                                  out_data, out_data_len );

      case CKM_DES3_CBC_PAD:
         return des3_cbc_pad_decrypt( sess,     length_only,
                                      ctx,
                                      in_data,  in_data_len,
                                      out_data, out_data_len );

      case CKM_RSA_PKCS:
         return rsa_pkcs_decrypt( sess,     length_only,
                                  ctx,
                                  in_data,  in_data_len,
                                  out_data, out_data_len );

      case CKM_RSA_X_509:
         return rsa_x509_decrypt( sess,     length_only,
                                  ctx,
                                  in_data,  in_data_len,
                                  out_data, out_data_len );
#ifndef NOAES
      case CKM_AES_CBC:
         return aes_cbc_decrypt( sess,     length_only,
                                 ctx,
                                 in_data,  in_data_len,
                                 out_data, out_data_len );

      case CKM_AES_ECB:
         return aes_ecb_decrypt( sess,     length_only,
                                 ctx,
                                 in_data,  in_data_len,
                                 out_data, out_data_len );

      case CKM_AES_CBC_PAD:
         return aes_cbc_pad_decrypt( sess,     length_only,
                                     ctx,
                                     in_data,  in_data_len,
                                     out_data, out_data_len );
#endif
      default:
         return CKR_MECHANISM_INVALID;
   }

   st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
   return CKR_FUNCTION_FAILED;
}


//
//
CK_RV
decr_mgr_decrypt_update( SESSION            *sess,
                         CK_BBOOL            length_only,
                         ENCR_DECR_CONTEXT  *ctx,
                         CK_BYTE            *in_data,
                         CK_ULONG            in_data_len,
                         CK_BYTE            *out_data,
                         CK_ULONG           *out_data_len )
{
   if (!sess || !in_data || !out_data || !ctx){
      st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
      return CKR_FUNCTION_FAILED;
   }
   if (ctx->active == FALSE){
      st_err_log(32, __FILE__, __LINE__);
      return CKR_OPERATION_NOT_INITIALIZED;
   }
   ctx->multi = TRUE;

   switch (ctx->mech.mechanism) {
      case CKM_CDMF_ECB:
      case CKM_DES_ECB:
         return des_ecb_decrypt_update( sess,     length_only,
                                        ctx,
                                        in_data,  in_data_len,
                                        out_data, out_data_len );

      case CKM_CDMF_CBC:
      case CKM_DES_CBC:
         return des_cbc_decrypt_update( sess,     length_only,
                                        ctx,
                                        in_data,  in_data_len,
                                        out_data, out_data_len );

      case CKM_DES_CBC_PAD:
      case CKM_CDMF_CBC_PAD:
         return des_cbc_pad_decrypt_update( sess,     length_only,
                                            ctx,
                                            in_data,  in_data_len,
                                            out_data, out_data_len );

      case CKM_DES3_ECB:
         return des3_ecb_decrypt_update( sess,     length_only,
                                         ctx,
                                         in_data,  in_data_len,
                                         out_data, out_data_len );

      case CKM_DES3_CBC:
         return des3_cbc_decrypt_update( sess,     length_only,
                                         ctx,
                                         in_data,  in_data_len,
                                         out_data, out_data_len );

      case CKM_DES3_CBC_PAD:
         return des3_cbc_pad_decrypt_update( sess,     length_only,
                                             ctx,
                                             in_data,  in_data_len,
                                             out_data, out_data_len );
#ifndef NOAES 
      case CKM_AES_ECB:
         return aes_ecb_decrypt_update( sess,     length_only,
                                        ctx,
                                        in_data,  in_data_len,
                                        out_data, out_data_len );

      case CKM_AES_CBC:
         return aes_cbc_decrypt_update( sess,     length_only,
                                        ctx,
                                        in_data,  in_data_len,
                                        out_data, out_data_len );

      case CKM_AES_CBC_PAD:
         return aes_cbc_pad_decrypt_update( sess,     length_only,
                                            ctx,
                                            in_data,  in_data_len,
                                            out_data, out_data_len );
#endif
      default:
         st_err_log(28, __FILE__, __LINE__);
         return CKR_MECHANISM_INVALID;
   }
   st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
   return CKR_FUNCTION_FAILED;
}


//
//
CK_RV
decr_mgr_decrypt_final( SESSION            *sess,
                        CK_BBOOL            length_only,
                        ENCR_DECR_CONTEXT  *ctx,
                        CK_BYTE            *out_data,
                        CK_ULONG           *out_data_len )
{
   if (!sess || !ctx){
      st_err_log(4, __FILE__, __LINE__, __FUNCTION__);
      return CKR_FUNCTION_FAILED;
   }
   if (ctx->active == FALSE){
      st_err_log(28, __FILE__, __LINE__);
      return CKR_OPERATION_NOT_INITIALIZED;
      }

   switch (ctx->mech.mechanism) {
      case CKM_CDMF_ECB:
      case CKM_DES_ECB:
         return des_ecb_decrypt_final( sess,     length_only,
                                       ctx,
                                       out_data, out_data_len );

      case CKM_CDMF_CBC:
      case CKM_DES_CBC:
         return des_cbc_decrypt_final( sess,     length_only,
                                       ctx,
                                       out_data, out_data_len );

      case CKM_DES_CBC_PAD:
      case CKM_CDMF_CBC_PAD:
         return des_cbc_pad_decrypt_final( sess,     length_only,
                                           ctx,
                                           out_data, out_data_len );

      case CKM_DES3_ECB:
         return des3_ecb_decrypt_final( sess,     length_only,
                                        ctx,
                                        out_data, out_data_len );

      case CKM_DES3_CBC:
         return des3_cbc_decrypt_final( sess,     length_only,
                                        ctx,
                                        out_data, out_data_len );

      case CKM_DES3_CBC_PAD:
         return des3_cbc_pad_decrypt_final( sess,     length_only,
                                            ctx,
                                            out_data, out_data_len );
#ifndef NOAES
      case CKM_AES_ECB:
         return aes_ecb_decrypt_final( sess,     length_only,
                                       ctx,
                                       out_data, out_data_len );

      case CKM_AES_CBC:
         return aes_cbc_decrypt_final( sess,     length_only,
                                       ctx,
                                       out_data, out_data_len );

      case CKM_AES_CBC_PAD:
         return aes_cbc_pad_decrypt_final( sess,     length_only,
                                           ctx,
                                           out_data, out_data_len );
#endif
      default:
         st_err_log(28, __FILE__, __LINE__);
         return CKR_MECHANISM_INVALID;
   }
   st_err_log(4, __FILE__, __LINE__, __FUNCTION__);

   return CKR_FUNCTION_FAILED;
}
