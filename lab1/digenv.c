/* 
KTH kursID: ID2200 - Operativsystem
Labb 1 - Pipelines

Skrivet av: 
Harald Vaksdal, haraldv@kth.se
Erik Bäck, erback@kth.se
(2104)

Det här programmet hämtar ut environmentvariabler med hjälp av pipes och processer.
*/

#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void newProcess(int processNo, int argc, char **argv);
void makePipe(int processNo);
pid_t startFork(void);
void closePipe(int processNo);
void pager();
void waitForProcess();

int activePipe[3][2];

int main(int argc, char **argv){
	int processNo = 0;
	newProcess(processNo, argc, argv);
	pager();
	return 0;
}

/**************************   Skapa processer   ***************************/
/*Använder sig av rekursion, dvs vi anropar denna från main endast en gång
Vi skapar upp child-processer och pipes för varje enskild exekvering vi gör
Beroende på vilken process vi är i gör vi de olika exekveringar. Vet inte 
om detta är det mest effektiva, men det gör i alla fall att vi kan använda
oss av rekursion.*/
void newProcess(int processNo, int argc, char **argv){
	pid_t child_pid;
	makePipe(processNo);
	child_pid = startFork();
	if (0 == child_pid){
		dup2(activePipe[processNo][1], STDOUT_FILENO); /*Dulicera skrivänden av den aktuella pipen*/
		closePipe(processNo);
		/*Processerna skapas nedan. Enligt "AnvandbaraSysanrop" behövs inte 
		någon else sats för att hantera att parent processen kommer efter.
		Därför kommer bara en child process skapas för att sedan gå vidare i 
		rekursionen och skapa upp nästa childprocess*/
		if (0 == processNo){
			execlp("printenv", "printenv", NULL);
			perror("Något blev fel med printenv"); exit(1);
		}
		else if (1 == processNo){
			if (1 < argc){
				execvp("grep", argv);
			}
			else{
				execvp("cat", argv);
			}
			perror("Grep funkade inte som den skulle");
			exit(1);
		}
		else if (2 == processNo){
			execlp("sort", "sort", NULL);
			perror("Nu var det fel på sort");
			exit(1);
		}
		else{
			perror("Nåt sket sig i if-satsen");
			exit(1);
		}
	}
	dup2(activePipe[processNo][0], STDIN_FILENO);/*Duplicera läsänden av den aktuella pipen*/
	closePipe(processNo);
	waitForProcess();
	processNo = processNo + 1;
	if (3 > processNo){
		newProcess(processNo, argc, argv);
	}
}

/************ Felhantering av pipes *************************/
void makePipe(int processNo){
  if (pipe(activePipe[processNo]) == -1)
    printf("Nu blev det fel på pipe nr: %d", processNo);
}

/********* Felhantering för skapande av processer **********/
/*Tar hand om eventuellt fel vid fork() och stänger i så fall
ner programmet*/
pid_t startFork(){
	pid_t child_pid = fork();
	if(child_pid < 0){
		printf("Fel vid skapande av process");
		exit(1);
	}
	return child_pid;
}

/************ Stängning av läs och skrivände i pipes ***********/
void closePipe(int processNo){
	close(activePipe[processNo][0]);
	close(activePipe[processNo][1]);
}

/****************** Kör exekvering med pager *******************/
void pager(){
	char *pager = getenv("PAGER");
	pager = (pager) ? pager : "less";
	execlp(pager, pager, NULL);
	perror("fel på pager");
}

/**************** Vänta in child processer *********************/
void waitForProcess(){
	/*Tagen från "AnvandbaraSysanrop med lite modifiering*/
	int status;
	wait(&status);
	if( WIFEXITED(status)){
		int child_status = WEXITSTATUS(status);
		if( 0 != child_status ) /*Problem med child process*/ {
		      fprintf( stderr, "Child process funkar inte. Felkod: %ld\n", (long int) child_status );
		      exit(1);
		} 
	}
}
