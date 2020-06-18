#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <linux/net.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __BIONIC__
/* should be in netinet/ip_icmp.h */
# define ICMP_DEST_UNREACH    3  /* Destination Unreachable  */
# define ICMP_SOURCE_QUENCH   4  /* Source Quench    */
# define ICMP_REDIRECT        5  /* Redirect (change route)  */
# define ICMP_ECHO            8  /* Echo Request      */
# define ICMP_TIME_EXCEEDED  11  /* Time Exceeded    */
# define ICMP_PARAMETERPROB  12  /* Parameter Problem    */
# define ICMP_TIMESTAMP      13  /* Timestamp Request    */
# define ICMP_TIMESTAMPREPLY 14  /* Timestamp Reply    */
# define ICMP_INFO_REQUEST   15  /* Information Request    */
# define ICMP_INFO_REPLY     16  /* Information Reply    */
# define ICMP_ADDRESS        17  /* Address Mask Request    */
# define ICMP_ADDRESSREPLY   18  /* Address Mask Reply    */
#endif

#define hehjtrace()  printf("trace l: %d\n", __LINE__)

enum {
	DEFDATALEN = 56,
	MAXIPLEN = 60,
	MAXICMPLEN = 76,
	MAX_DUP_CHK = (8 * 128),
	MAXWAIT = 10,
	PINGINTERVAL = 1, /* 1 second */
	pingsock = 0,
};
#define BB_LITTLE_ENDIAN 1

uint16_t inet_cksum(uint16_t *addr, int nleft)
{
    unsigned sum = 0;
    while (nleft > 1) {
        sum += *addr++;
        nleft -= 2;
    }   

    /* Mop up an odd byte, if necessary */
    if (nleft == 1) {
        if (BB_LITTLE_ENDIAN)
            sum += *(uint8_t*)addr;
        else
            sum += *(uint8_t*)addr << 8;
    }   

    /* Add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */

    return (uint16_t)~sum;
}

void xdup2(int from, int to) 
{
    if (dup2(from, to) != to) {
        fprintf(stderr, "can't duplicate file descriptor");
        exit(-1);
    }
}


void xmove_fd(int from, int to) 
{
    if (from == to) 
        return;
    xdup2(from, to);
    close(from);
}

/* Die with an error message if sendto failed.
 *  * Return bytes sent otherwise  */
ssize_t xsendto(int s, const void *buf, size_t len, const struct sockaddr *to,
        socklen_t tolen)
{
    ssize_t ret = sendto(s, buf, len, 0, to, tolen);
    if (ret < 0) {
        close(s);
        fprintf(stderr, "sendto");
        exit(-1);
    }
    return ret;
}


static void
#if ENABLE_PING6
create_icmp_socket(len_and_sockaddr *lsa)
#else
create_icmp_socket(void)
#define create_icmp_socket(lsa) create_icmp_socket()
#endif
{
	int sock;
#if ENABLE_PING6
	if (lsa->u.sa.sa_family == AF_INET6)
		sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	else
#endif
		sock = socket(AF_INET, SOCK_RAW, 1); /* 1 == ICMP */
	if (sock < 0) {
		if (errno != EPERM) {
			// bb_perror_msg_and_die(bb_msg_can_not_create_raw_socket);
            fprintf(stderr, "can't create raw socket");
            exit(-1);
        }
#if defined(__linux__) || defined(__APPLE__)
		/* We don't have root privileges.  Try SOCK_DGRAM instead.
		 * Linux needs net.ipv4.ping_group_range for this to work.
		 * MacOSX allows ICMP_ECHO, ICMP_TSTAMP or ICMP_MASKREQ
		 */
#if ENABLE_PING6
		if (lsa->u.sa.sa_family == AF_INET6)
			sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
		else
#endif
			sock = socket(AF_INET, SOCK_DGRAM, 1); /* 1 == ICMP */
		if (sock < 0)
#endif
		// bb_error_msg_and_die(bb_msg_perm_denied_are_you_root);
        fprintf(stderr, "permission denied (are you root?)");
        exit(-1);
	}

	xmove_fd(sock, pingsock);
}

/* Simple version */

struct globals {
	char *hostname;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];
} FIX_ALIASING;

struct globals G;

#ifndef BUFSIZ
# define BUFSIZ 4096
#endif
enum { COMMON_BUFSIZE = (BUFSIZ >= 256*sizeof(void*) ? BUFSIZ+1 : 256*sizeof(void*)) };
char bb_common_bufsiz1[COMMON_BUFSIZE];

typedef struct len_and_sockaddr {
    socklen_t len; 
    union {
        struct sockaddr sa;
        struct sockaddr_in sin; 
#if ENABLE_FEATURE_IPV6
        struct sockaddr_in6 sin6;
#endif
    } u; 
} len_and_sockaddr;
enum {
    LSA_LEN_SIZE = offsetof(len_and_sockaddr, u),
    LSA_SIZEOF_SA = sizeof(
            union {
            struct sockaddr sa;
            struct sockaddr_in sin; 
#if ENABLE_FEATURE_IPV6
            struct sockaddr_in6 sin6;
#endif
            }    
            )    
};



static void noresp(int ign)
{
	printf("No response from %s\n", G.hostname);
	exit(EXIT_FAILURE);
}

static void ping4(len_and_sockaddr *lsa)
{
	struct icmp *pkt;
	int c;

	pkt = (struct icmp *) G.packet;
	/*memset(pkt, 0, sizeof(G.packet)); already is */
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_cksum = inet_cksum((uint16_t *) pkt, sizeof(G.packet));

	xsendto(pingsock, G.packet, DEFDATALEN + ICMP_MINLEN, &lsa->u.sa, lsa->len);

	/* listen for replies */
	while (1) {
#if 0
		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);

		c = recvfrom(pingsock, G.packet, sizeof(G.packet), 0,
				(struct sockaddr *) &from, &fromlen);
#else
		c = recv(pingsock, G.packet, sizeof(G.packet), 0);
#endif
		if (c < 0) {
			if (errno != EINTR)
				fprintf(stderr, "recvfrom");
			continue;
		}
		if (c >= 76) {			/* ip + icmp */
			struct iphdr *iphdr = (struct iphdr *) G.packet;

			pkt = (struct icmp *) (G.packet + (iphdr->ihl << 2));	/* skip ip hdr */
			if (pkt->icmp_type == ICMP_ECHOREPLY)
				break;
		}
	}
	// if (ENABLE_FEATURE_CLEAN_UP)
		close(pingsock);
}

static len_and_sockaddr* str2sockaddr(const char * host)
{
    int rc; 
    len_and_sockaddr *r; 
    struct addrinfo *result = NULL;
    struct addrinfo *used_res;
    const char *org_host = host; /* only for error msg */
    const char *cp;
    struct addrinfo hint;

    struct in_addr in4;
    if (inet_aton(host, &in4) != 0) {
        r = malloc(LSA_LEN_SIZE + sizeof(struct sockaddr_in));
        memset(r, 0, sizeof(*r));
        r->len = sizeof(struct sockaddr_in);
        r->u.sa.sa_family = AF_INET;
        r->u.sin.sin_addr = in4;
    }
    else {
        memset(&hint, 0 , sizeof(hint));
        hint.ai_family = AF_INET;
        /* Need SOCK_STREAM, or else we get each address thrice (or more)
         *      * for each possible socket type (tcp,udp,raw...): */
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_flags = 0;
        rc = getaddrinfo(host, NULL, &hint, &result);
        if (rc || !result) {
            fprintf(stderr, "bad address '%s'", org_host);
            exit(-1);
        }
        used_res = result;

        r = malloc(LSA_LEN_SIZE + used_res->ai_addrlen);
        memset(r, 0, LSA_LEN_SIZE + used_res->ai_addrlen);
        r->len = used_res->ai_addrlen;
        memcpy(&r->u.sa, used_res->ai_addr, used_res->ai_addrlen);
    }

    if (result)
        freeaddrinfo(result);
    return r;
}

static int common_ping_main()
{
	len_and_sockaddr *lsa;
    printf("ping %s\n", G.hostname);
    lsa = str2sockaddr(G.hostname);
    char* a = (char *)&lsa->u.sa + 4;
    printf("ip: %d.%d.%d.%d\n", a[0], a[1], a[2], a[3]);

	/* Set timer _after_ DNS resolution */
	signal(SIGALRM, noresp);
	alarm(5); /* give the host 5000ms to respond */

	create_icmp_socket(lsa);
#if ENABLE_PING6
	if (lsa->u.sa.sa_family == AF_INET6)
		ping6(lsa);
	else
#endif
		ping4(lsa);
	printf("%s is alive!\n", G.hostname);
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    int n = argc - 1;
    if (n <= 0) {
        fprintf(stderr, "usage:\n\tping <hostname>\n");
        exit(-1);
    }

    memset(&G, 0, sizeof(G));
    G.hostname = argv[n];
    return common_ping_main();
}

