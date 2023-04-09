# Tema 1 PCom (Router)

* Popescu Mihai Costel-324CD

Flow-ul temei:

1. Initializam si citim din fisier ```rout_table``` apoi cream memoria cache a protocolilui ARP,
   cat si coada in care vor fi tinute pachetele care vor trebui trimise ulterior, iar apoi vom sorta
   descrescator dupa rezultatul operatiei ```&``` dintre masca si prefixul a doua intrari din tabela
   de rutare ,iar daca aceste campuri sunt egale de sorteaza dupa masca.
2. Verificarea tipului pachetului primit:
    * IPV4:
        * Verificam daca IP-ul destinatie aflat in header-ul IPV4 este chiar IP-ul routerului daca da
          se va trimite un pachet ICMP de tip reply. Altfel trebui interpretat pachetul primit,
          vom verifica checksum-ul pachetului daca acesta este gresit se va arunca si se trece la
          urmatorul pachet, apoi se verifica ttl-ul daca este mai mic sau egal cu 1 inseamna ca timpul
          de viata al pachetului a expirat si trebuie eliminat dupa care se va trimite un mesaj
          ICMP ```Time exceeded```.
          Se actualizeaza checksum-ul. Apoi se cauta ruta cea mai buna catre destinatie ,daca nu se gaseste
          se trimite un mesaj ICMP ```Destination unreachable```. Se verifica daca adresa IP a urmatorului
          hop este in memoria cache pentru a lua adresa MAC a acesteia daca nu este se va trimite un
          protocol de tip ARP request pentru a afla adresa MAC. Apoi se va actualiza adresa MAC destinatie din
          header-ul eth al pachetului primit cu adresa mac optinuta fie din memoria cache fie da raspuns al request-ului
          , iar adresa sursa va deveni adresa MAC a router-ului. La final se trimite pachetul pe interfata gasita in cautarea
          celei mai bune rute.
    * ARP:
        * In cadrul protocolului ARP sunt 3 cazuri :
          * Atunci cand trebuie sa fie trimis un ARP request trebuie ca pachetul care are nevoie
          de adresa MAC sa fie bagat in coada, iar apoi pachetul ARP trebuie sa aiba in eth header
          ca sursa adresa MAC a interfetei pe care trebuie trimis, iar ca destinatie adresa de broadcast,
          iar in headerul ARP adresa MAC sursa este adresa MAC a interfetei pe care trebuie trimis 
          aresa MAC destinatie este prina de zerouri , adresa IP sursa este IP interfetei pe care trebuie trimis
          , iar IP ul destinate este IP ul urmatorului hop.La final se trimite pachetul pe interfata gasita in cautarea
            celei mai bune rute.
          * Atunci cand se primeste un pachet de tip ARP request trebuie sa inversam adresele sursa cu 
          adresele destinatie si sa setam in headerul ARP ```arp_hdr->op = htons(2)```.
          * Atunci cand se primeste un pachet de tip ARP reply se va adauga in memoria cache adresele IP si MAC sursa
          , iar a apoi se vor trimite pachetele care se afla in coada.
    * ICMP:
        * In cadrup protocolului ICMP trebuie sa cream un pachet ICMP care va avea ca adrese de destinatie
        adresele sursa ale pachetului care a primit o eroare si hostul care a trimis acest pachet trebuie notificat
        iar adresele sursa sunt adresele routerului. Iar in headerul ICMP trebuie setat si tipul corespunzator erorii
    
3. LPM (Longest prefix match):
   * Am implementat LPM-ul sub forma unei cautari binare in care elimin treptat ultimii biti din masca cat si din destinatie
   , iar apoi verific daca operatia ```&``` dintre masca si prefix este egala cu destinatia mea curenta, iar daca da 
   verificam daca masca mea curenta cea cu bitii eliminati este egala cu masca din tabela daca acestes sunt 
   egale am gasit cea mai buna ruta , iar daca masca din tabela este mai mare limita superioara devine mijlocul, 
   respectiv daca este mai mica marginea inferioara devine mijloc + 1.
          