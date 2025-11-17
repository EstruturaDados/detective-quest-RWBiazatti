#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================== UTILS ============================== */

#ifndef _MSC_VER
  #define CLEAR() printf("\033[2J\033[H")
#else
  #include <windows.h>
  void CLEAR(){ system("cls"); }
#endif

static char *strdup_safe(const char *s){
    if(!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if(p) memcpy(p, s, n);
    return p;
}

/* Remove trailing newline from fgets */
static void chomp(char *s){
    if(!s) return;
    size_t n = strlen(s);
    if(n && (s[n-1] == '\n' || s[n-1] == '\r')) s[n-1] = '\0';
}

/* Read line from stdin safely */
static void read_line(const char *prompt, char *buf, size_t bufsz){
    if(prompt) printf("%s", prompt);
    if(fgets(buf, (int)bufsz, stdin) == NULL){
        buf[0] = '\0';
        return;
    }
    chomp(buf);
}

/* Lowercase copy for case-insensitive compare */ 
static void strtolower_inplace(char *s){
    for(; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

/* ============================== MAPA (ÁRVORE BINÁRIA) ==============================
 * Cada nó representa um cômodo. left/right podem simbolizar, por exemplo, "esquerda/direita"
 * ou qualquer bifurcação do mapa. Mantemos também um ID amigável e nome do cômodo.
 */

typedef struct RoomNode {
    int id;
    char *name;
    struct RoomNode *left;
    struct RoomNode *right;
} RoomNode;

/* Cria um nó de sala */
static RoomNode* room_create(int id, const char *name){
    RoomNode *r = (RoomNode*)calloc(1, sizeof(RoomNode));
    if(!r) return NULL;
    r->id = id;
    r->name = strdup_safe(name);
    r->left = r->right = NULL;
    return r;
}

/* Libera o mapa recursivamente */
static void room_free(RoomNode *root){
    if(!root) return;
    room_free(root->left);
    room_free(root->right);
    free(root->name);
    free(root);
}

/* Exibe em ordem (simples visualização da estrutura do mapa) */
static void room_print_inorder(RoomNode *root, int depth){
    if(!root) return;
    room_print_inorder(root->left, depth+1);
    for(int i=0;i<depth;i++) printf("  ");
    printf("• [%d] %s\n", root->id, root->name);
    room_print_inorder(root->right, depth+1);
}

/* Constrói um mapa exemplo de mansão (pode ser personalizado) */
static RoomNode* build_sample_mansion(void){
    /* Estrutura:
             1 Hall
            /       \
         2 Sala    3 Biblioteca
        /   \        /        \
       4 Cozinha 5 Jantar   6 Escritório   7 Conservatório
    */
    RoomNode *n1 = room_create(1, "Hall");
    RoomNode *n2 = room_create(2, "Sala de Estar");
    RoomNode *n3 = room_create(3, "Biblioteca");
    RoomNode *n4 = room_create(4, "Cozinha");
    RoomNode *n5 = room_create(5, "Sala de Jantar");
    RoomNode *n6 = room_create(6, "Escritório");
    RoomNode *n7 = room_create(7, "Conservatório");

    n1->left = n2;     n1->right = n3;
    n2->left = n4;     n2->right = n5;
    n3->left = n6;     n3->right = n7;
    return n1;
}

/* ============================== PISTAS (BST) ==============================
 * BST por string (case-insensitive). Cada nó guarda a "clue" (pista).
 */

typedef struct ClueNode {
    char *clue;
    struct ClueNode *left;
    struct ClueNode *right;
} ClueNode;

static int clue_cmp(const char *a, const char *b){
    /* Case-insensitive lexicographical */
    for(;; a++, b++){
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if(ca != cb) return ca - cb;
        if(ca == 0) return 0;
    }
}

static ClueNode* clue_create(const char *clue){
    ClueNode *n = (ClueNode*)calloc(1, sizeof(ClueNode));
    if(!n) return NULL;
    n->clue = strdup_safe(clue);
    return n;
}

static void clue_free(ClueNode *root){
    if(!root) return;
    clue_free(root->left);
    clue_free(root->right);
    free(root->clue);
    free(root);
}

static ClueNode* clue_insert(ClueNode *root, const char *clue){
    if(!root) return clue_create(clue);
    int c = clue_cmp(clue, root->clue);
    if(c < 0) root->left  = clue_insert(root->left, clue);
    else if(c > 0) root->right = clue_insert(root->right, clue);
    /* duplicado: ignora */
    return root;
}

static ClueNode* clue_min(ClueNode *n){
    while(n && n->left) n = n->left;
    return n;
}

static ClueNode* clue_delete(ClueNode *root, const char *clue){
    if(!root) return NULL;
    int c = clue_cmp(clue, root->clue);
    if(c < 0) root->left  = clue_delete(root->left, clue);
    else if(c > 0) root->right = clue_delete(root->right, clue);
    else {
        if(!root->left){
            ClueNode *r = root->right;
            free(root->clue); free(root);
            return r;
        } else if(!root->right){
            ClueNode *l = root->left;
            free(root->clue); free(root);
            return l;
        } else {
            ClueNode *m = clue_min(root->right);
            free(root->clue);
            root->clue = strdup_safe(m->clue);
            root->right = clue_delete(root->right, m->clue);
        }
    }
    return root;
}

static int clue_search(ClueNode *root, const char *clue){
    if(!root) return 0;
    int c = clue_cmp(clue, root->clue);
    if(c == 0) return 1;
    if(c < 0) return clue_search(root->left, clue);
    return clue_search(root->right, clue);
}

static void clue_print_inorder(ClueNode *root){
    if(!root) return;
    clue_print_inorder(root->left);
    printf(" - %s\n", root->clue);
    clue_print_inorder(root->right);
}

/* ============================== TABELA HASH (CLUE -> SUSPEITO) ==============================
 * Hash por string, com encadeamento separado.
 */

typedef struct MapKV {
    char *key;        /* clue */
    char *value;      /* suspect */
    struct MapKV *next;
} MapKV;

typedef struct HashMap {
    size_t buckets;
    MapKV **table;
} HashMap;

static unsigned long djb2(const char *str){
    unsigned long hash = 5381;
    int c;
    while((c = (unsigned char)*str++)){
        hash = ((hash << 5) + hash) + (unsigned long)c; /* hash * 33 + c */
    }
    return hash;
}

static HashMap* hashmap_create(size_t buckets){
    HashMap *m = (HashMap*)calloc(1, sizeof(HashMap));
    if(!m) return NULL;
    m->buckets = (buckets ? buckets : 53);
    m->table = (MapKV**)calloc(m->buckets, sizeof(MapKV*));
    if(!m->table){ free(m); return NULL; }
    return m;
}

static void hashmap_free(HashMap *m){
    if(!m) return;
    for(size_t i=0;i<m->buckets;i++){
        MapKV *p = m->table[i];
        while(p){
            MapKV *nx = p->next;
            free(p->key);
            free(p->value);
            free(p);
            p = nx;
        }
    }
    free(m->table);
    free(m);
}

static void hashmap_put(HashMap *m, const char *key, const char *value){
    if(!m || !key) return;
    unsigned long h = djb2(key) % m->buckets;
    MapKV *p = m->table[h];
    for(; p; p = p->next){
        if(strcasecmp(p->key, key) == 0){
            /* atualiza */
            free(p->value);
            p->value = strdup_safe(value);
            return;
        }
    }
    /* insere na cabeça */
    MapKV *n = (MapKV*)calloc(1, sizeof(MapKV));
    n->key = strdup_safe(key);
    n->value = strdup_safe(value);
    n->next = m->table[h];
    m->table[h] = n;
}

static const char* hashmap_get(HashMap *m, const char *key){
    if(!m || !key) return NULL;
    unsigned long h = djb2(key) % m->buckets;
    MapKV *p = m->table[h];
    for(; p; p = p->next){
        if(strcasecmp(p->key, key) == 0) return p->value;
    }
    return NULL;
}

static int hashmap_del(HashMap *m, const char *key){
    if(!m || !key) return 0;
    unsigned long h = djb2(key) % m->buckets;
    MapKV *p = m->table[h], *prev = NULL;
    while(p){
        if(strcasecmp(p->key, key) == 0){
            if(prev) prev->next = p->next; else m->table[h] = p->next;
            free(p->key); free(p->value); free(p);
            return 1;
        }
        prev = p; p = p->next;
    }
    return 0;
}

static void hashmap_print(HashMap *m){
    if(!m) return;
    for(size_t i=0;i<m->buckets;i++){
        MapKV *p = m->table[i];
        while(p){
            printf(" [%s] -> %s\n", p->key, p->value);
            p = p->next;
        }
    }
}

/* ============================== JOGO/DEMO ============================== */

typedef struct Game {
    RoomNode *mansion_root;
    RoomNode *current_room;
    ClueNode *clues_bst;
    HashMap  *clue2sus;
} Game;

static Game* game_create(void){
    Game *g = (Game*)calloc(1, sizeof(Game));
    if(!g) return NULL;
    g->mansion_root = build_sample_mansion();
    g->current_room = g->mansion_root;
    g->clues_bst = NULL;
    g->clue2sus = hashmap_create(101);
    return g;
}

static void game_free(Game *g){
    if(!g) return;
    room_free(g->mansion_root);
    clue_free(g->clues_bst);
    hashmap_free(g->clue2sus);
    free(g);
}

static void print_header(void){
    printf("=====================================================\n");
    printf("           Detective Quest — Console Demo            \n");
    printf("  Map (Binary Tree) | Clues (BST) | Links (HashMap)  \n");
    printf("=====================================================\n");
}

/* Navegação simples no mapa binário: left/right */
static void action_show_map(Game *g){
    printf("\nMapa (in-order, indentação ~ profundidade):\n");
    room_print_inorder(g->mansion_root, 0);
    printf("\nVocê está em: [%d] %s\n", g->current_room->id, g->current_room->name);
    printf("Mover (L)eft/(R)ight, (B)ack to menu? ");
    char op[16]; read_line("", op, sizeof(op));
    if(op[0]=='L' || op[0]=='l'){
        if(g->current_room->left){
            g->current_room = g->current_room->left;
        }else{
            printf("Não há caminho à esquerda.\n");
        }
    }else if(op[0]=='R' || op[0]=='r'){
        if(g->current_room->right){
            g->current_room = g->current_room->right;
        }else{
            printf("Não há caminho à direita.\n");
        }
    }
}

/* Operações com pistas (BST) */
static void action_clues(Game *g){
    for(;;){
        printf("\n--- Pistas (BST) ---\n");
        printf("1) Inserir pista\n");
        printf("2) Remover pista\n");
        printf("3) Buscar pista\n");
        printf("4) Listar em ordem\n");
        printf("0) Voltar\n");
        printf("Escolha: ");
        char op[8]; read_line("", op, sizeof(op));
        if(op[0]=='0') break;
        if(op[0]=='1'){
            char buf[256]; read_line("Digite a pista: ", buf, sizeof(buf));
            if(buf[0]){
                g->clues_bst = clue_insert(g->clues_bst, buf);
                printf("Pista inserida.\n");
            }
        }else if(op[0]=='2'){
            char buf[256]; read_line("Pista a remover: ", buf, sizeof(buf));
            g->clues_bst = clue_delete(g->clues_bst, buf);
            printf("Remoção concluída (se existia).\n");
        }else if(op[0]=='3'){
            char buf[256]; read_line("Pista a buscar: ", buf, sizeof(buf));
            printf("%s\n", clue_search(g->clues_bst, buf) ? "Encontrada." : "Não encontrada.");
        }else if(op[0]=='4'){
            printf("Pistas em ordem:\n");
            clue_print_inorder(g->clues_bst);
        }
    }
}

/* Operações com Tabela Hash para vincular pista->suspeito */
static void action_links(Game *g){
    for(;;){
        printf("\n--- Vincular Pista a Suspeito (Hash) ---\n");
        printf("1) Relacionar/Atualizar (pista -> suspeito)\n");
        printf("2) Consultar suspeito pela pista\n");
        printf("3) Remover relação por pista\n");
        printf("4) Listar todos\n");
        printf("0) Voltar\n");
        printf("Escolha: ");
        char op[8]; read_line("", op, sizeof(op));
        if(op[0]=='0') break;
        if(op[0]=='1'){
            char key[256], val[256];
            read_line("Pista: ", key, sizeof(key));
            read_line("Suspeito: ", val, sizeof(val));
            if(key[0] && val[0]){
                hashmap_put(g->clue2sus, key, val);
                printf("Vinculado: [%s] -> %s\n", key, val);
            }
        }else if(op[0]=='2'){
            char key[256];
            read_line("Pista: ", key, sizeof(key));
            const char *sus = hashmap_get(g->clue2sus, key);
            if(sus) printf("Suspeito: %s\n", sus);
            else    printf("Sem vínculo para esta pista.\n");
        }else if(op[0]=='3'){
            char key[256];
            read_line("Pista a desvincular: ", key, sizeof(key));
            printf("%s\n", hashmap_del(g->clue2sus, key) ? "Removido." : "Pista não encontrada.");
        }else if(op[0]=='4'){
            printf("Vínculos pista -> suspeito:\n");
            hashmap_print(g->clue2sus);
        }
    }
}

/* Pequena simulação de dedução final */
static void action_deduce(Game *g){
    (void)g;
    printf("\n--- Dedução Final ---\n");
    printf("Use as funções de pistas e vínculos para montar as evidências.\n");
    printf("Quando tiver os vínculos adequados, consulte cada pista para obter o suspeito.\n");
    printf("Esta demo não implementa lógica de verificação de 'culpado oficial',\n");
    printf("mas fornece as estruturas para você estender.\n");
}

/* ============================== MAIN LOOP ============================== */

int main(void){
    Game *game = game_create();
    if(!game){
        fprintf(stderr, "Falha ao iniciar jogo.\n");
        return 1;
    }

    for(;;){
        print_header();
        printf("Local atual: [%d] %s\n", game->current_room->id, game->current_room->name);
        printf("\nMenu Principal:\n");
        printf("1) Mostrar/Explorar mapa (Árvore Binária)\n");
        printf("2) Gerenciar pistas (BST)\n");
        printf("3) Vincular pista->suspeito (Hash)\n");
        printf("4) Dedução final (demo)\n");
        printf("0) Sair\n");
        printf("Escolha: ");
        char op[8]; read_line("", op, sizeof(op));
        if(op[0]=='0') break;
        else if(op[0]=='1') action_show_map(game);
        else if(op[0]=='2') action_clues(game);
        else if(op[0]=='3') action_links(game);
        else if(op[0]=='4') action_deduce(game);
        printf("\nPressione ENTER para continuar...");
        char tmp[4]; fgets(tmp, sizeof(tmp), stdin); /* pausa simples */
        CLEAR();
    }

    game_free(game);
    return 0;
}