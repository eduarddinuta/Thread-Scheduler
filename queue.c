#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct thread_t thread_t;

typedef struct ll_node_t {
	thread_t *data;
	struct ll_node_t *next;

} ll_node_t;



typedef struct linked_list_t {
	ll_node_t *head;
	unsigned int size;

} linked_list_t;



linked_list_t *ll_create(void)
{
	linked_list_t *list = (linked_list_t *)malloc(sizeof(linked_list_t));
	
	list->head = NULL;
	list->size = 0;

	return list;
}


ll_node_t *create_node(linked_list_t* list, thread_t* new_data)
{
	ll_node_t *tmp = malloc(sizeof(*tmp));

	tmp->data = new_data;
	tmp->next = NULL;

	return tmp;
}

 /*Pe baza datelor trimise prin pointerul new_data, se creeaza un nou nod care e
 * adaugat pe pozitia n a listei reprezentata de pointerul list. Pozitiile din
 * lista sunt indexate incepand cu 0 (i.e. primul nod din lista se afla pe
 * pozitia n=0). Daca n >= nr_noduri, noul nod se adauga la finalul listei).
 */

void ll_add_nth_node(linked_list_t* list, unsigned int n, thread_t *new_data)
{
	list->size++;
	ll_node_t *new, *curr, *prev;
	curr = list->head;
	prev = NULL;

	while (curr != NULL && n > 0) {
		prev = curr;
		curr = curr->next;
		n--;
	}

	new = malloc(sizeof(ll_node_t));
	new->data = new_data;
	new->next = NULL;

	if (prev == NULL) {
		new->next = list->head;
		list->head = new;
		return;
	}

	prev->next = new;
	new->next = curr;
}



/*
 * Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Daca n >= nr_noduri - 1, se elimina nodul de
 * la finalul listei. Functia intoarce un pointer spre acest nod proaspat
 * eliminat din lista. Este responsabilitatea apelantului sa elibereze memoria
 * acestui nod.
 */

ll_node_t* ll_remove_nth_node(linked_list_t* list, unsigned int n)
{
	if (list->head == NULL)
		return NULL;

	list->size--;
	ll_node_t *curr, *prev;
	curr = list->head;
	prev = NULL;

	while (curr->next != NULL && n > 0) {
		prev = curr;
		curr = curr->next;
		n--;
	}

	if (prev == NULL)
		list->head = curr->next;
	else
		prev->next = curr->next;

	return curr;

}



/*
 * Functia intoarce numarul de noduri din lista al carei pointer este trimis ca
 * parametru.
 */

unsigned int ll_get_size(linked_list_t* list)
{
	return list->size;
}



/*
 * Procedura elibereaza memoria folosita de toate nodurile din lista, iar la
 * sfarsit, elibereaza memoria folosita de structura lista si actualizeaza la
 * NULL valoarea pointerului la care pointeaza argumentul (argumentul este un
 * pointer la un pointer).
 */

void ll_free(linked_list_t** pp_list)
{
	ll_node_t *curr = (*pp_list)->head;
	while (curr != NULL) {
		free(curr->data);
		ll_node_t *tmp = curr->next;
		free(curr);
		curr = tmp;
	}

	free(*pp_list);
}

void enqueue(linked_list_t *list, thread_t *newNode) {
	ll_add_nth_node(list, list->size + 1, newNode);
}

ll_node_t* dequeue(linked_list_t *list) {
	return ll_remove_nth_node(list, 0);
}
