#include <has.h>

#include <openssl/bio.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

static const char *hexdigits = "0123456789ABCDEF";
static void hexdump(char *out, unsigned char *in, size_t l)
{
    size_t i, j;
    for(i = 0, j = 0; i < l; i++, j += 2) {
        unsigned char c = in[i];
        out[j]     = hexdigits[c >> 4];
        out[j + 1] = hexdigits[c & 0xF];
    }
    out[j] = '\0';
}

static int time2string(ASN1_TIME *t, char **r, int *l) {
    BIO   *buf = BIO_new(BIO_s_mem());
    char  *b, *data;
    size_t s; 

    if(r == NULL || l == NULL) {
        return -1;
    }

    ASN1_TIME_print(buf, t);
    s = BIO_get_mem_data(buf, &b);
    if((data = calloc(s, 1)) == NULL) {
        return -1;
    }
    
    memcpy(data, b, s);
    *r = data;
    *l = s;

    BIO_free(buf);
    return 0;
}

char *oid2string(ASN1_OBJECT *oid)
{
    if(oid == NULL) {
        return NULL;
    } else {
        int l = OBJ_obj2txt(NULL, 0, oid, 0);
        char *s;
        s = malloc(l + 2);
        OBJ_obj2txt(s, l + 1, oid, 0);
        return s;
    }
}

char *asn1string(ASN1_STRING *val)
{
    char *str, *t;
    int l;
    BIO  *buf = BIO_new(BIO_s_mem());
    ASN1_STRING_print_ex(buf, val, 0 /* ASN1_STRFLGS_ESC_CTRL | */
                         /* ASN1_STRFLGS_UTF8_CONVERT | */
                         /* ASN1_STRFLGS_ESC_MSB */
                         );
    l = BIO_get_mem_data(buf, &t);
    str = calloc(l + 1, 1);
    memcpy(str, t, l);
    BIO_free(buf);
    return str;
}

char *ext_dump(X509_EXTENSION *ex)
{
    char *str, *t;
    int l;
    BIO  *buf = BIO_new(BIO_s_mem());
    X509V3_EXT_print(buf, ex, 0, 0);
    l = BIO_get_mem_data(buf, &t);
    str = calloc(l + 1, 1);
    memcpy(str, t, l);
    BIO_free(buf);
    return str;
}


has_t *dn2hash(X509_NAME *dn)
{
    has_t *name = has_array_new(8), *e;
    int i, j = X509_NAME_entry_count(dn);

    for(i = 0; i < j; i++) {
        X509_NAME_ENTRY *ne = X509_NAME_get_entry(dn, i);

        if(ne) {
            char *k = oid2string(X509_NAME_ENTRY_get_object(ne));
            char *v = asn1string(X509_NAME_ENTRY_get_data(ne));
            if(k && v) {
                e = has_hash_new(2);
                has_hash_set_str_o(e, k, has_string_new_str_o(v, 1), 1);
                has_array_push(name, e);
            }
        }
    }

    return name;
}

char *dn2string(X509_NAME *dn)
{
    char *str, *t;
    int l;
    BIO  *buf = BIO_new(BIO_s_mem());

    /* ASN1_STRFLGS_RFC2253 */
    X509_NAME_print_ex(buf, dn, 0,
                       XN_FLAG_SEP_COMMA_PLUS |
                       XN_FLAG_DN_REV |
                       XN_FLAG_FN_SN |
                       XN_FLAG_DUMP_UNKNOWN_FIELDS );
    l = BIO_get_mem_data(buf, &t);
    str = calloc(l + 1, 1);
    memcpy(str, t, l);
    BIO_free(buf);
    return str;
}

has_t *has_x509_extensions(STACK_OF(X509_EXTENSION) *exts)
{
    has_t *r = has_hash_new(16);
    int i, j = sk_X509_EXTENSION_num(exts);

    for(i = 0; i < j; i++) {
        has_t *cur = has_hash_new(4);
		X509_EXTENSION *ex = sk_X509_EXTENSION_value(exts, i);
        ASN1_OBJECT *obj = X509_EXTENSION_get_object(ex);
        has_t *critical = has_int_new(X509_EXTENSION_get_critical(ex) ? 1 : 0);
        has_hash_set_str(cur, "critical", critical);
        has_hash_set_str(cur, "content", has_string_new_str_o(ext_dump(ex), 1));
        has_hash_set_str_o(r, oid2string(obj), cur, 1);
    }
    return r;
}

has_t *has_x509_new(X509 *x509)
{
    has_t     *crt  = NULL;
    has_t     *r    = NULL;
    X509_CINF *ci   = NULL;
    size_t     l;
    int        i, j;

    crt = has_hash_new(32);

    /* Version */
    l = X509_get_version(x509);
    r = has_hash_set_str(crt, "version", has_int_new(l));

    /* Serial number */
    if(r) {
        ASN1_INTEGER *sn = X509_get_serialNumber(x509);
        char *serial = calloc(2 * sn->length + 1, 1);
        hexdump(serial, sn->data, sn->length);
        r = has_hash_set_str(crt, "serial", has_string_new_str_o(serial, 1));
    }

    ci = x509->cert_info;
    if(r) {
        char *sigalg;
        i = OBJ_obj2txt(NULL, 0, ci->signature->algorithm, 0);
        sigalg =  malloc(i + 2);
        j = OBJ_obj2txt(sigalg, i + 1, ci->signature->algorithm, 0);
        r = has_hash_set_str(crt, "sig_alg", has_string_new_str_o(sigalg, 1));
    }

    if(r) {
        char *notbefore, *notafter;
        int lb, la;

        time2string(X509_get_notBefore(x509), &notbefore, &lb);
        time2string(X509_get_notAfter(x509), &notafter, &la);
        r = has_hash_set_str(crt, "not_before",
                             has_string_new_o(notbefore, lb, 1));
        r = has_hash_set_str(crt, "not_after",
                             has_string_new_o(notafter, la, 1));
    }

    if(r) {
        has_t *subject = has_hash_new(2);
        char *dn = dn2string(X509_get_subject_name(x509));
        has_hash_set_str(subject, "string", has_string_new_str_o(dn, 1));
        has_hash_set_str(subject, "struct", dn2hash(X509_get_subject_name(x509)));
        r = has_hash_set_str(crt, "subject", subject);
    }

    if(r) {
        has_t *issuer = has_hash_new(2);
        char *dn = dn2string(X509_get_issuer_name(x509));
        has_hash_set_str(issuer, "string", has_string_new_str_o(dn, 1));
        has_hash_set_str(issuer, "struct", dn2hash(X509_get_issuer_name(x509)));
        r = has_hash_set_str(crt, "issuer", issuer);
    }

    if(r) {
        has_t *extensions = has_x509_extensions(ci->extensions);
        r = has_hash_set_str(crt, "extensions", extensions);
    }

    return crt;
}
