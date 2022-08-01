/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

void echo(int connfd);
typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
}pool;

int byte_cnt = 0;

typedef struct stock {
    int ID;
    int left_stock;
    int price;
    int readcnt;
    struct stock* left_node;
    struct stock* right_node;
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
void init_pool(int listenfd, pool* p)
{
    int i;
    p->maxi = -1;
    for (i = 0;i < FD_SETSIZE; i++) {
        p->clientfd[i] = -1;
    }
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}
void add_client(int connfd, pool* p)
{
    int i;
    p->nready--;
    for (i = 0;i < FD_SETSIZE;i++) {
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }
        if (i == FD_SETSIZE)
            app_error("add_client error: Too many clinets\n");
    }
}
char str[MAXLINE];
void show(item* tree) {

    if (tree == NULL)
        return;
    char temp[10];
    sprintf(temp, "%d", tree->ID);
    strcat(temp, " ");
    strcat(str, temp);

    sprintf(temp, "%d", tree->left_stock);
    strcat(temp, " ");
    strcat(str, temp);

    sprintf(temp, "%d",tree->price);
    strcat(temp, "\n");
    strcat(str, temp);
    
    show(tree->left_node);
    show(tree->right_node);

    return;

}
int buy(item* tree, int id, int amount)
{
    if (tree == NULL)
        return -1;
    if (tree->ID == id) {
        if (tree->left_stock < amount)
        {
            return 0;
        }
        else
            tree->left_stock -= amount;
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
            tree->left_stock += amount;
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
void check_clients(pool* p, item *tree)
{
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for (i = 0;(i <= p->maxi) && (p->nready > 0);i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
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
                }
                else if (!strncmp(buf, "sell", 4))
                {
                    str[0] = '\0';
                    int request_id, request_amount;
                    sscanf(buf, "%*s %d %d", &request_id, &request_amount);
                    sell(tree, request_id, request_amount);
        
                    strcpy(str, "[sell] success\n");
                    Rio_writen(connfd, str, MAXLINE);
 
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
            else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }

}
int main(int argc, char **argv) 
{
    Signal(SIGINT, sig_int_handler);

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;

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
    init_pool(listenfd, &pool);
    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);
        if(FD_ISSET(listenfd,&pool.ready_set))
        {
            clientlen = sizeof(struct sockaddr_storage); 
	        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(connfd, &pool);
        }
        check_clients(&pool, tree);
	    //echo(connfd);
    }
    exit(0);
}
/* $end echoserverimain */
