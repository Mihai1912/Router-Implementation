#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include "stdio.h"
#include "string.h"
#include "arpa/inet.h"

typedef struct node {
    struct route_table_entry *route;
    struct node *left;
    struct node *right;
} node;

typedef struct q_elem {
    char buf[MAX_PACKET_LEN];
    int size;
} q_elem;

struct route_table_entry *cautare(struct route_table_entry *route_table, int route_table_len, uint32_t dest) {
    for (int i = 0; i < 32; i++) {
        uint32_t my_mask = 0xffffffff;
        my_mask = (my_mask << i) >> i;
        uint32_t cur_dest = (dest << i) >> i;
        int start = 0;
        int stop = route_table_len - 1;
        while (start < stop) {
            int mij = (start + stop) / 2;
            if ((route_table[mij].mask & route_table[mij].prefix) == cur_dest) {
                if (my_mask == route_table[mij].mask) {
                    return &(route_table[mij]);
                } else if (my_mask < route_table[mij].mask) {
                    stop = mij;
                } else {
                    start = mij + 1;
                }
            } else if ((route_table[mij].mask & route_table[mij].prefix) > cur_dest) {
                stop = mij;
            } else {
                start = mij + 1;
            }
        }
    }
    return NULL;
}

int comaratator(const void *obj1, const void *obj2) {
    struct route_table_entry *ob1 = (struct route_table_entry *) obj1;
    struct route_table_entry *ob2 = (struct route_table_entry *) obj2;

    if ((ob1->mask & ob1->prefix) > (ob2->mask & ob2->prefix)) {
        return 1;
    } else if ((ob1->mask & ob1->prefix) == (ob2->mask & ob2->prefix)) {
        if (ob1->mask > ob2->mask) {
            return 1;
        } else if (ob1->mask < ob2->mask) {
            return -1;
        }
        return 0;
    }
    return -1;
}

int ipStrinToInt(char *addr) {
    int ip_int = 0;
    char *token;

    token = strtok(addr, ".");
    while (token != NULL) {
        ip_int = (ip_int << 8) + atoi(token);
        token = strtok(NULL, ".");
    }
    return ip_int;
}

void send_ICMP(char *buf, uint8_t type, int interface) {

    struct ether_header *eth_hdr = (struct ether_header *) buf;
    struct iphdr *ip_hdr = (struct iphdr *) (buf + sizeof(struct ether_header));


    char icmp_buff[sizeof(struct ether_header) + 2 * sizeof(struct iphdr) + sizeof(struct icmphdr)];
    char buff[64 + sizeof(struct iphdr)];
    memcpy(buff, ip_hdr, 64 + sizeof(struct iphdr));

    struct ether_header *icmp_eth_hdr = malloc(sizeof(struct ether_header));
    struct iphdr *icmp_ip_hdr = malloc(sizeof(struct iphdr));
    struct icmphdr *icmp_hdr = malloc(sizeof(struct icmphdr));

    memcpy(icmp_eth_hdr, eth_hdr, sizeof(struct ether_header));
    memcpy(icmp_ip_hdr, ip_hdr, sizeof(struct iphdr));

    for (int i = 0; i < 6; i++) {
        icmp_eth_hdr->ether_dhost[i] = eth_hdr->ether_shost[i];
        icmp_eth_hdr->ether_shost[i] = eth_hdr->ether_dhost[i];
    }
    icmp_eth_hdr->ether_type = htons(0x0800);

    icmp_ip_hdr->daddr = ip_hdr->saddr;
    icmp_ip_hdr->saddr = ip_hdr->daddr;
    icmp_ip_hdr->protocol = 1;
    icmp_ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));

    icmp_hdr->code = 0;
    icmp_hdr->type = type;
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = htons(checksum((uint16_t *) icmp_hdr, sizeof(struct icmphdr)));

    icmp_hdr->un.echo.id = 0;
    icmp_hdr->un.echo.sequence = 0;
    icmp_hdr->un.frag.mtu = 0;
    icmp_hdr->un.frag.__unused = 0;
    icmp_hdr->un.gateway = 0;

    memcpy(icmp_buff, icmp_eth_hdr, sizeof(struct ether_header));
    memcpy(icmp_buff + sizeof(struct ether_header), icmp_ip_hdr, sizeof(struct iphdr));
    memcpy(icmp_buff + sizeof(struct ether_header) + sizeof(struct iphdr), icmp_hdr, sizeof(struct icmphdr));
    memcpy(icmp_buff + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr), buff,
           sizeof(struct iphdr));


    send_to_link(interface, icmp_buff, sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr));
}


int main(int argc, char *argv[]) {
    char buf[MAX_PACKET_LEN];

    // Do not modify this line
    init(argc - 2, argv + 2);

    struct route_table_entry route_table[100000];
    int rtable_len = read_rtable(argv[1], route_table);

    struct arp_entry arp_cache[100000];
    int arp_cache_size = 0;

    queue q = queue_create();
    int q_size = 0;

    qsort(route_table, rtable_len, sizeof(struct route_table_entry), comaratator);

    while (1) {

        int interface;
        size_t len;

        interface = recv_from_any_link(buf, &len);
        q_elem *qElem = malloc(sizeof(q_elem));
        memcpy(qElem->buf, buf, len);
        qElem->size = len;

        DIE(interface < 0, "recv_from_any_links");

        struct ether_header *eth_hdr = (struct ether_header *) buf;
        /* Note that packets received are in network order,
        any header field which has more than 1 byte will need to be conerted to
        host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
        sending a packet on the link, */

        uint8_t *macRouter = malloc(6 * (sizeof(uint8_t)));
        get_interface_mac(interface, macRouter);


        if (ntohs(eth_hdr->ether_type) == 0x0800) { //IPv4
            struct iphdr *ip_hdr = (struct iphdr *) (buf + sizeof(struct ether_header));
            char *addr_router = get_interface_ip(interface);
            int routerIP = htonl(ipStrinToInt(addr_router));

            if (ip_hdr->daddr == routerIP) {
                send_ICMP(buf, 0, interface);
                continue;

            } else {
                uint16_t old_checksum = ntohs(ip_hdr->check);
                ip_hdr->check = 0;
                uint16_t new_checksum = checksum((uint16_t *) ip_hdr, sizeof(struct iphdr));

                if (old_checksum != new_checksum) {
                    printf("wrong_checksum\n");
                    continue;
                }

                if (ip_hdr->ttl <= 1) {
                    send_ICMP(buf, 11, interface);
                    continue;
                }

                ip_hdr->ttl--;

                uint16_t new_check = checksum((uint16_t *) ip_hdr, sizeof(struct iphdr));
                ip_hdr->check = ntohs(new_check);

                struct route_table_entry *a = cautare(route_table, rtable_len, ip_hdr->daddr);

                if (a == NULL) {
                    send_ICMP(buf, 3, interface);
                    continue;
                }

                struct arp_entry *arp = NULL;
                for (int i = 0; i < arp_cache_size; i++) {
                    if (ntohl(a->next_hop) == ntohl(arp_cache[i].ip)) {
                        arp = &(arp_cache[i]);
                        break;
                    }
                }

                if (arp == NULL) {
                    queue_enq(q, qElem);
                    q_size++;

                    char req_packet[sizeof(struct ether_header) + sizeof(struct arp_header)];

                    struct ether_header *eth_req = malloc(sizeof(struct ether_header));

                    eth_req->ether_type = htons(0x0806);
                    get_interface_mac(a->interface, eth_req->ether_shost);
                    for (int i = 0; i < 6; i++) {
                        eth_req->ether_dhost[i] = 0xFF;
                    }

                    struct arp_header *arp_req = malloc(sizeof(struct arp_header));
                    arp_req->htype = htons(1);
                    arp_req->ptype = htons(2048);
                    arp_req->hlen = 6;
                    arp_req->plen = 4;
                    arp_req->op = htons(1);

                    get_interface_mac(a->interface, arp_req->sha);
                    arp_req->spa = htonl(ipStrinToInt(get_interface_ip(a->interface)));

                    arp_req->tpa = a->next_hop;
                    for (int i = 0; i < 6; i++) {
                        arp_req->tha[i] = 0x00;
                    }

                    memcpy(req_packet, eth_req, sizeof(struct ether_header));
                    memcpy(req_packet + sizeof(struct ether_header), arp_req, sizeof(struct arp_header));

                    send_to_link(a->interface, req_packet, sizeof(struct ether_header) + sizeof(struct arp_header));
                    continue;
                }

                for (int i = 0; i < 6; i++) {
                    eth_hdr->ether_dhost[i] = arp->mac[i];
                    eth_hdr->ether_shost[i] = macRouter[i];
                }

                send_to_link(a->interface, buf, len);
                continue;
            }

        } else if (ntohs(eth_hdr->ether_type) == 0x0806) { // arp
            struct arp_header *arp_hdr = (struct arp_header *) (buf + sizeof(struct ether_header));

            if (ntohs(arp_hdr->op) == 1) {         //ARP request

                if (ntohl(arp_hdr->tpa) != ipStrinToInt(get_interface_ip(interface))) {
                    continue;
                }

                arp_hdr->op = htons(2);

                uint32_t a = arp_hdr->spa;
                arp_hdr->spa = arp_hdr->tpa;
                arp_hdr->tpa = a;

                for (int i = 0; i < 6; i++) {
                    arp_hdr->tha[i] = arp_hdr->sha[i];
                    eth_hdr->ether_dhost[i] = arp_hdr->sha[i];
                }

                get_interface_mac(interface, arp_hdr->sha);
                get_interface_mac(interface, eth_hdr->ether_shost);


                send_to_link(interface, buf, len);
                continue;

            } else if (ntohs(arp_hdr->op) == 2) {  //ARP reply

                int cont = 0;

                for (int i = 0; i < arp_cache_size; i++) {
                    if (arp_cache[i].ip == arp_hdr->spa) {
                        cont = 1;
                    }
                }

                if (cont) {
                    continue;
                }

                arp_cache[arp_cache_size].ip = arp_hdr->spa;
                for (int i = 0; i < 6; i++) {
                    arp_cache[arp_cache_size].mac[i] = eth_hdr->ether_shost[i];
                }
                arp_cache_size++;

                if (queue_empty(q)) {
                    continue;
                }

                q_elem *q_elem_deq = (q_elem *) queue_deq(q);
                char new_packet[MAX_PACKET_LEN];
                memcpy(new_packet, q_elem_deq->buf, q_elem_deq->size);

                struct ether_header *packet_eth_hdr = (struct ether_header *) new_packet;
                struct iphdr *packet_ip_hdr = (struct iphdr *) (new_packet + sizeof(struct ether_header));

                struct route_table_entry *a = cautare(route_table, rtable_len, packet_ip_hdr->daddr);

                if (a == NULL) {
                    send_ICMP(buf, 3, interface);
                    continue;
                }

                if (a != NULL) {

                    packet_ip_hdr->ttl--;
                    packet_ip_hdr->check = 0;
                    packet_ip_hdr->check = ntohs(checksum((uint16_t *) packet_ip_hdr, sizeof(struct iphdr)));

                    for (int i = 0; i < 6; i++) {
                        packet_eth_hdr->ether_dhost[i] = arp_hdr->sha[i];
                    }
                    get_interface_mac(a->interface, packet_eth_hdr->ether_shost);


                    send_to_link(a->interface, new_packet, q_elem_deq->size);
                }
                continue;
            }
        }
    }
}

