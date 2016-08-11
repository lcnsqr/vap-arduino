// Utiliza realloc()
#include <stdlib.h>

// Ponteiro para tarefa
typedef struct agndTarefa_s * agndTarefa_p;
// Tarefa
struct agndTarefa_s {
	void (*acao)(agndTarefa_p);
	unsigned long intervalo;
	char ativo;
	int estado;
	unsigned long contador;
};

// Ponteiro para lista de tarefas
typedef struct agndTarefas_s * agndTarefas_p;
// Lista de tarefas
struct agndTarefas_s {
	int total;
	agndTarefa_p * tarefa;
	unsigned long passado;
	void (*incluir)(agndTarefas_p, agndTarefa_p );
	void (*executar)(agndTarefas_p, unsigned long );
};
void agndIncluir(agndTarefas_p I, agndTarefa_p agndTarefa){
	I->tarefa = (agndTarefa_p*) realloc(I->tarefa, sizeof(agndTarefa_p) * (I->total + 1));
	I->tarefa[I->total] = agndTarefa;
	I->total++;
}
// Execucao das tarefas agendadas
void agndExecutar(agndTarefas_p I, unsigned long agora){
	if ( agora - I->passado == 0 ) return;
	I->passado = agora;
	for ( unsigned int t = 0; t < I->total; t++ ){
		if ( I->tarefa[t]->ativo == 1 ){
			if ( I->tarefa[t]->contador >= I->tarefa[t]->intervalo ){
				(*(I->tarefa[t]->acao))(I->tarefa[t]);
				I->tarefa[t]->contador = 0;
			}
			I->tarefa[t]->contador++;
		}
	}
}
//struct agndTarefas_s agndTarefas = { .incluir = agndIncluir, .executar = agndExecutar };
struct agndTarefas_s agndTarefas = { 0, 0, 0, agndIncluir, agndExecutar };

