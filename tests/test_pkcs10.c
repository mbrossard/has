#include "has.c"
#include "has_json.c"
#include "has_x509.c"

#include <stdio.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/engine.h>

has_t *has_pkcs10_new(X509_REQ *pkcs10);

void openssl_init()
{
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();
    ERR_load_crypto_strings();
}

void openssl_cleanup()
{
    CONF_modules_unload(1);
    OBJ_cleanup();
    EVP_cleanup();
    ENGINE_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_state(0);
    ERR_free_strings();
}

int main(int argc, char **argv)
{
    X509_REQ  *pkcs10 = NULL;
    BIO       *bio    = NULL;
    has_t     *p10    = NULL;
    char      *json   = NULL; 
    size_t l;

    openssl_init();

    if ((bio = BIO_new(BIO_s_file())) == NULL) {
        return -1;
    }

    if(argc < 2) {
        BIO_set_fp(bio, stdin, BIO_NOCLOSE);
    } else {
        BIO_read_filename(bio, argv[1]);
    }
    
    /* Format DER */
    if((pkcs10 = d2i_X509_REQ_bio(bio, NULL)) == NULL) {
        ERR_clear_error();
        /* Format PEM */
        pkcs10 = PEM_read_bio_X509_REQ(bio, NULL, NULL, NULL);
    }

    if(!pkcs10) {
        fprintf(stderr, "Error loading certificate\n");
        return -1;
    }

    if((p10 = has_pkcs10_new(pkcs10)) == NULL) {
        fprintf(stderr, "Error converting certificate\n");
        return -1;
    } 

    if(has_json_serialize(p10, &json, &l, HAS_JSON_SERIALIZE_PRETTY) == 0) {
        printf("%s\n", json);
        free(json);
    } else {
        fprintf(stderr, "Error serializing certificate\n");
        return -1;        
    }

    has_free(p10);
    X509_REQ_free(pkcs10);
    BIO_free(bio);

    openssl_cleanup();

    return 0;
}
