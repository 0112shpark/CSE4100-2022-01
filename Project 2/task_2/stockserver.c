/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
# define SBUFSIZE 1000
# define NTHREADS 100
void echo(int connfd);

int byte_cnt = 0;

typedef struct {
    int* buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
}sbuf_t;

sbuf_t sbuf;
void sbuf_init(sbuf_t* sp, int n) {
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}
void sbuf_deinit(sbuf_t* sp)
{
    Free(sp->buf);
}
void sbuf_insert(sbuf_t* sp, int item)
{
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}
int sbuf_remove(sbuf_t* sp)
{
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}
typedef struct stock {
    int ID;
    int left_stock;
    int price;
    struct stock* left_node;
    struct stock* right_node;
    int readcnt;
    sem_t mutex;
    sem_t w;
}item;

item *tree;
int n_node=0;
item* new_node(int id, int left, int price) {
    item* temp = (item*)malloc(sizeof(item));
    temp->ID = id;
    temp->price = price;
    temp->left_stock = left;
    temp->left_node = NULL;
    temp->right_node = NULL;
    Sem_init(&(temp->mutex), 0, 1);
    Sem_init(&(temp->w), 0, 1);
    n_node++;
    return temp;
}
item *insert(item *tree, int id, int left, int price)
{
    if (tree == NULL) {
       
        return new_node(id, left, price);
    }

    else {  
        if (id < tree->ID)
        {
         
            tree->left_node = insert(tree->left_node, id, left, price);
        }
        else
            tree->right_node = insert(tree->right_node, id, left, price);
       
    }
    return tree;
}
void free_tree(item* tree) {
    if (tree == NULL) return;
    free_tree(tree->left_node);
    free_tree(tree->right_node);
    free(tree);
}
void write_data(item* tree, FILE* fp) {
    if (tree == NULL) return;
    fprintf(fp, "%d %d %d\n", tree->ID, tree->left_stock, tree->price);
    write_data(tree->left_node, fp);
    write_data(tree->right_node, fp);

}
void sig_int_handler(int sig)
{
    
    FILE* fp = fopen("stock.txt", "w");
    write_data(tree, fp);
    fclose(fp);
    free_tree(tree);
    printf("exit server\n");
    exit(0);

}

char str[MAXLINE];
void show(item* tree) {

    if (tree == NULL)
        return;
    char temp[10];
    P(&(tree->mutex));
    tree->readcnt++;
    if (tree->readcnt == 1)
        P(&(tree->w));
    V(&(tree->mutex));

    sprintf(temp, "%d", tree->ID);
    strcat(temp, " ");
    strcat(str, temp);

    sprintf(temp, "%d", tree->left_stock);
    strcat(temp, " ");
    strcat(str, temp);

    sprintf(temp, "%d",tree->price);
    strcat(temp, "\n");
    strcat(str, temp);
    
    P(&(tree->mutex));
    tree->readcnt--;
    if (tree->readcnt == 0)
        V(&(tree->w));
    V(&(tree->mutex));

    show(tree->left_node);
    show(tree->right_node);

    return;

}
int buy(item* tree, int id, int amount)
{
    if (tree == NULL)
        return -1;
    if (tree->ID == id) {
        P(&(tree->mutex));
        tree->readcnt++;
        if (tree->readcnt == 1)
            P(&(tree->w));
        V(&(tree->mutex));
        if (tree->left_stock < amount)
        {
            P(&(tree->mutex));
            tree->readcnt--;
            if (tree->readcnt == 0)
                V(&(tree->w));
            V(&(tree->mutex));
            return 0;
        }
        else { 
            P(&(tree->mutex));
            tree->readcnt--;
            if (tree->readcnt == 0)
                V(&(tree->w));
             V(&(tree->mutex));

            P(&(tree->w));
             tree->left_stock -= amount;
            V(&(tree->w));
        }
       
        return 1;
    }
    else if (id < tree->ID)
    {
        buy(tree->left_node, id,amount);
    }
    else if (id > tree->ID)
    {
         buy(tree->right_node, id, amount);
    }
    return -1;
}
void sell(item* tree, int id, int amount)
{
    if (tree == NULL)
        return;
    if (tree->ID == id) {
       /*P(&(tree->mutex));
        tree->readcnt++;
        if (tree->readcnt == 1)
            P(&(tree->w));
        V(&(tree->mutex));
        
        P(&(tree->mutex));
        tree->readcnt--;
        if (tree->readcnt == 0)
            V(&(tree->w));
        V(&(tree->mutex));*/

        P(&(tree->w));
        tree->left_stock += amount;
        V(&(tree->w));

        

        return;
    }
    else if (id < tree->ID)
    {
        sell(tree->left_node, id, amount);
    }
    else if (id > tree->ID)
    {
        sell(tree->right_node, id, amount);
    }
    return;
}
void order(int connfd)
{
    int  n;
    char buf[MAXLINE];
    rio_t rio;
 
    rio_readinitb(&rio, connfd);
 
            while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd%d\n", n, byte_cnt, connfd);
                if (!strcmp(buf, "show\n"))
                {
                    str[0] = '\0';
                    show(tree);
                    Rio_writen(connfd, str, MAXLINE);

                   
                }
                else if (!strncmp(buf, "buy", 3))
                {
                    str[0] = '\0';
                    int request_id, request_amount;
                    sscanf(buf, "%*s %d %d", &request_id, &request_amount);
                    int result = buy(tree, request_id, request_amount);
                    if (result)
                    {   strcpy(str, "[buy] success\n");
                        Rio_writen(connfd, str, MAXLINE);
                    }
                    else
                    {
                        strcpy(str, "Not enough left stock\n");
                        Rio_writen(connfd, str, MAXLINE);
                    }
                    V(&tree->mutex);
                }
                else if (!strncmp(buf, "sell", 4))
                {
                    str[0] = '\0';
                    int request_id, request_amount;
                    sscanf(buf, "%*s %d %d", &request_id, &request_amount);
                    sell(tree, request_id, request_amount);
        
                    strcpy(str, "[sell] success\n");
                    Rio_writen(connfd, str, MAXLINE);
                    V(&tree->mutex);
    
                }
                else if (!strcmp(buf, "exit\n"))
                {
                    FILE* fp = fopen("stock.txt", "w");
                    write_data(tree, fp);
                    fclose(fp);
                    free_tree(tree);
                    printf("exit server\n");
                    exit(0);
                }
            }
            


}
void* thread(void* vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        order(connfd);
        Close(connfd);
    }
    
}
int main(int argc, char **argv) 
{
    Signal(SIGINT, sig_int_handler);

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    FILE* fp = fopen("stock.txt", "r");
    int id, left, price;
    while (fscanf(fp, "%d %d %d\n", &id, &left, &price) != EOF) {
        tree = insert(tree, id, left, price);
    }
    fclose(fp);
    
    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    for (int i = 0;i < NTHREADS;i++) {
        Pthread_create(&tid, NULL, thread, NULL);
    }
    while (1) {
            clientlen = sizeof(struct sockaddr_storage); 
	        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            sbuf_insert(&sbuf, connfd);
    }
    exit(0);
}
/* $end echoserverimain */
