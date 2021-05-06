Tema2PC - aplicatie client server

Structura temei:
Client - Subscriber:
Clientul deschide un socket si se conecteaza la server. Mai apoi clientul trimite
catre server un mesaj cu id-ul lui a.i. serverul va putea sa il inregistreze.
In cazul in care 2 clienti se conecteaza cu acelasi ID, clientul va primi un mesaj
gol iar mai apoi clientul curent va fi inchis.

Activitatile clientului:
1. Clientul trimite mesaje catre server. Mainly, clientul vrea sa se inregistreze/
dezaboneze de la un topic. Acest lucru se poate realiza cu mesaje catre server pe
care serverul le va analiza si pune in practica. Serverul stie sa faca diferenta
dintre mesaje printr-un flag trimis de catre client cu 1 sau 2.
2. Clientul va analiza comenzile de la tastatura de catre user.
Aplicatia client va prelua de la stdin inputul de la user, analizand cuvintele cheie.
De exemplu: subscribe topic 1 este corect dar si subscribe topic 1 asdas este corect,
extra caracterele fiind ignorate.

Comanda de subscribe:
	- Comanda va contine cuvantul cheie : subscribe, topicul la care userul va
	vrea sa se inregistreze si un SF(store and forward), flag cu care clientul
	va putea primi mesaje si cand este deconectat(mai exact, cand se va reconecta
	le va primi).
Comanda de unsubscribe:
	- Contine cuvantul cheie unsubscribe dar si topicul la care vrea sa se deza-
	boneze. Va fi trimis catre server.
Comanda de exit:
	- Contine cuvantul cheie "exit" care va inchide aplicatia client.
3. Clientul va primi mesaje de la server. Aceste mesaje sunt de fapt notificarile de
la topicurile la care userul este abonat.Clientul va primii ip-ul si port-ul de la
clientul UDP dar si mesajul primit pentru topicul respectiv.
Clientul este inchis cu comanda de shutdown in locul la close, deoarece in cazul
in care mai exista mesaje in buffer ele vor putea fi trimise pana la final.

Server - managerul aplicatiei

Serverul deschide doua socketuri:
	1.TCP, asculta pentru noii clienti tcp si deschide un nou socket pentru ei.
	2.UDP, receptioneaza mesajele de la clientii UDP.

Activitatile serverului:
1. Serverul manageriaza spatiul de memorie al aplicatiei.
Cautam ca aplicatia sa fie cat mai rapida, nu ne dorim intarzieri inutile, asa ca,
serverul metine socketurile asociate cu id-ul clinetului intr un hashmap deoarece
cautarea este in O(1).
Fiecare id al clientului este asociat cu o structura pentru datele TCP.
La fel, cautarea va fi in O(1). Metine id-ul clientului cu un vector de mesaje pentru
sf-ul 0 tot intr-o mapa, astfel, cautarea este tot in O(1).
Fiecare topic va avea asociat o lista cu id-ul clientilor dar si cu SF-ul lor.
Astfel, cand o notificare va trebui trimisa, cautarea clientului este tot in O(1).

2.Serverul manageriaza clientii.
Daca se primesc date pe socketul TCP pe care se asculta pentru clienti, atunci serverul
accepta conexiunea si da turn off la algoritmul lui Neagle. Apoi asteapta mesaje de la client.
Se uita dupa client cu ajutorul structurilor de date.Daca clientul exista dar este offline,
atunci clientul este marcat online si mesajele din lista vor fi trimise. Daca clientul
este deja online serverul inchide socketul. In acest caz, niciodata nu vor exista 2 clienti
cu acelasi ID. Nu vor exista chei duplicate.

3.Serverul manageriaza mesaje TCP.
 -comanda de abonare. Serverul se uita dupa topic in baza de date si va initia un nou loc
pentru client. Daca topicul exista se va verifica lista de abonati. Daca un client este
abonat deja la un topic si cere abonarea la un alt topic se va face inlocuirea. Daca este
deja abonat va fi ignorat. Daca un client este un nou subscriber va fi adaugat in lista.
In acest caz vom fi asigurati ca un client nu va fi abonat la acelasi topic de doua ori.
Aceste operatii vor fi in O(n) din cauza cautarii clientului in lista de abonati.
 - Comanda de unsubscribe: Se va cauta topicul. Daca nu exista va fi ignorat. In alt caz,
id ul clientului va fi cautat in lista de abonati a topicului si sters. Aceste operatii
vor lua O(n) din cauza cautarii clientului.
 -  Mesaj gol: client deconectat.

4.Daca se primesc date pe socketul UDP, va fi tinut intr-un buffer care va fi parsat
mai departe. va fi cautat topicul in vaza de date si daca nu exista este ignorat,
inseamna ca nu are niciun subscriber. Altfel, un mesaj va fi compus pentru clienti.
Pentru fiecare abonat, trimite mesajul doar daca sunt ONLINE sau daca sunt OFFLINE
si au SF-ul 1.
	