#include "mp3.h"

extern node_t *head;

void del(char *name){
	node_t *temp =  head;
	node_t *prev = NULL;

	if(temp != NULL && (strcmp(temp->name, name) == 0)){
		head = temp->next;
		free(temp->name);
		free(temp);
		return;
	}
	while(temp != NULL && (strcmp(temp->name, name) != 0)){
		prev = temp;
		temp = temp->next;
	}
	if(temp == NULL){
		return;
	}
	prev->next = temp->next;
	free(temp->name);
	free(temp);

}
