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

has_t *has_x509_new(X509 *x509);

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
    X509  *x509 = NULL;
    BIO   *bio  = NULL;
    has_t *crt  = NULL;
    char  *json = NULL; 
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
    if((x509 = d2i_X509_bio(bio, NULL)) == NULL) {
        ERR_clear_error();
        BIO_reset(bio);
        /* Format PEM */
        x509 = PEM_read_bio_X509_AUX(bio, NULL, NULL, NULL);
    }

    if(!x509) {
        fprintf(stderr, "Error loading certificate\n");
        return -1;
    }

    if((crt = has_x509_new(x509)) == NULL) {
        fprintf(stderr, "Error converting certificate\n");
        return -1;
    } 

    if(has_json_serialize(crt, &json, &l, HAS_JSON_SERIALIZE_PRETTY) == 0) {
        printf("%s\n", json);
        free(json);
    } else {
        fprintf(stderr, "Error serializing certificate\n");
        return -1;        
    }

    has_free(crt);
    X509_free(x509);
    BIO_free(bio);

    openssl_cleanup();

    return 0;
}
