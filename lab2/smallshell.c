/* 
KTH kursID: ID2200 - Operativsystem
Labb 2 - Pipelines

Skrivet av: 
Harald Vaksdal, haraldv@kth.se
Erik Bäck, erback@kth.se
(2104)

Det här programmet skapar en egen kommandotolk
*/
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>


#define MAXINPUT 71 /*Inte längre intput än detta, enligt labbinstruktioner*/
#define MAXARG 6 /*Inte mer argument än detta, enligt labbinstruktioner*/
#define RUNNING 1 /*Används bara för att kolla om programmet ska köras eller stängas*/
#define	CLOSING 0

void prompt();
void changeDirectory(char *arguments);
bool checkProcessType(char *arguments);
void executeCommand(bool bgProcess, char **arguments);
pid_t startFork();
void regSigHandler(int sigCode);
void sigHandler(int sigCode);
void cleanBgProcess();


int main(){
	int programStatus = RUNNING; /*Används för att kolla om kommandotolken ska fortsätta köra*/
	printf("\n\n*** Välkommen till vår egna kommandotolk ***\n******* För att avlsuta skriv 'exit' *******\n");
	char input[MAXINPUT];
	regSigHandler(SIGINT); /*Blockerar CTRL-C*/

	/**************** Kommandotolksloopen ********************/
	while(programStatus){
		char **arguments = (char **) calloc (MAXARG,  MAXINPUT);
	  	int length = 0;
	  	char *args;

		prompt(); /*Skriver ut vår kommandutolk*/

	  	/*Hämta in det som användaren skriver. Därefter delas den upp i de olika argument som skickats med*/
		fgets(input, MAXINPUT, stdin);
	    args = strtok(input, " \n\n");
	    while( args != NULL){
	      arguments[length] = args; 
	      args = strtok(NULL, " ");
	      length++;
	    }

	    cleanBgProcess(); /*Kolla om det finns avslutade bakgrundsprocesser*/
	    /*Gör ingenting om det inte finns några argument;*/
	    if (NULL == arguments[0]){
	    	
	    }
	    /*Man får inte bara skicka ett &*/
	    else if (strcmp(arguments[0], "&") == 0){
	    	printf("Du får inte bara skicka ett &...");
	    }
	    /*Hanterar att man ska stänga ner kommandotolk. Ändrar programStatus till CLOSING*/
	    else if (strcmp(arguments[0], "exit") == 0){
	    	printf("Stänger ner kommandotolken..\n");
	    	programStatus = CLOSING;
	    }
	    /*Kollar om man skrivit cd och skickar då vidare den till den funktionen*/
	    else if (strcmp(arguments[0], "cd") == 0){
            changeDirectory(arguments[1]);
	    }
	    /*Hanterar övriga kommandon*/
	    else{
	    	bool bgProcess = checkProcessType(arguments[length-1]);/*Kollar om det ska vara bakgrundsprocess*/
	    	if (bgProcess){
	    		/*Plockar bort &-tecknet eftersom det inte ska användas mer*/
	    		if (strcmp(arguments[length-1], "&") == 0){
	    			arguments[length-1] = NULL;
	    		}
	    		else{
	    			strtok(arguments[length-1], "&");
	    		}
	    	}
	    	/*Kör kommandot i funktionen executeCommand*/
	    	executeCommand(bgProcess, arguments);
	    }
	    free(arguments); /*Släpper argumenten i minnet*/
	}
}

/***************** Simulera kommandotolk ***********************/
/*Skriver ut vår kommandotolk. Hämtar in nuvarande plats och skriver 
ut den.*/
void prompt() {
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
  	printf("\n %s >> ", cwd);
  	fflush(stdout);
}

/**************************** CD *******************************/
/*Kör enbart kommandot cd med efterföljande argument (felkontroll om
det inte skulle fungera). Finns inga argumentändras mappen till hem-
mappen.*/
void changeDirectory(char * arguments){
	if (NULL == arguments){
  		char *homeDir = getenv("HOME");
    	chdir(homeDir);
    }
  	else{
  		strtok(arguments, "\n");
    	if(0 != chdir(arguments)){
    		printf("Angiven plats finns inte");
    	}
  	}
}

/**************** Kontrollera processtyp *********************/
/*Kontrollerar om det finns ett & tecken antingen i slutet på sista ordet 
eller som sista separat tecken. Utnyttjar att vi redan tidigare har delat 
upp input strängen och tar därför bara in sista argumentet som parameter
och kollar. Returnerar bara en bool*/
bool checkProcessType(char *argument){
	strtok(argument, "\n");
	int argLen = strlen(argument);
	if (strcmp(&argument[argLen -1], "&") == 0 || strcmp(argument, "&") == 0){
		return true;
	}
	else{
		return false;
	}
}

/******************* Kommandon andrar än cd *****************************/
/*I denna funktion hanteras alla kommandon som inte är cd. Det är uppdelat i
två delar. Första delen startar upp en child process och kör det kommando 
användaren har skrivit in. Därefter kollar vi om det är en bakgrunsprocess
eller förgrundsprocess. Är det en bakgrundsprocess skriver vi enbart ut 
att processen har startat (nerstägning av bakgrundsproceserna hanteras i en
annan funktion som vi kör varje gång man går in i kommandotolksloopen.
Är det en förgrundsprocess väntar vi in att den ska avlsutas innan vi går 
vidare i programmet. Vi räknar även ut hur lång tid processen tog och 
beroende på om processen avlsutades själv eller med en annan signal än 0
skriver vi ut hur den avslutades.*/
void executeCommand(bool bgProcess, char **arguments){
	struct timeval start, end;
	double elapsed;
	gettimeofday(&start, NULL); /*Hämta in aktuell tid för senare uträkning*/
	pid_t childProcess = startFork(); /*Skapa processen*/
	if (0 == childProcess){
		execvp(arguments[0], arguments); /*Exekvera kommandot med argument*/
		perror("Nu va det något galet med det du skrev in...");
		exit(1);
	}
	/*Om det är en bakgrundsprocess:*/
	if (bgProcess){
		printf("Bakgrundsprocess startad med process ID: %d\n", childProcess);

	}
	/*Om det är en förgrundsprocess*/
	else{
		int status;
		printf("Förgrundsprocess startad med process ID: %d\n", childProcess);
		waitpid(childProcess, &status, 0); /*Väntar in att processen ska bli klar*/
		gettimeofday(&end, NULL); /*Hämta in aktuell tid för tidsberäknings nedan*/
		elapsed = (end.tv_sec - start.tv_sec) * 1000000;
		elapsed = (elapsed + (end.tv_usec - start.tv_usec)) / 1000;
		if (0 == status){
			printf("Förgrundsprocess %d avslutad \n", childProcess);
			printf("Tid för process: %f ms", elapsed);
		}
		else{
			printf("Förgrundsprocess %d avlsutad med signal %d\n", childProcess, status);
			printf("Tid för process: %f ms", elapsed);
		}
	}
}


/*************** Starta ny process **********************/
/*Startat process på ett säkert sätt. Om något går fel med fork() 
kommer programmet att avsluta här*/
pid_t startFork(){
	pid_t child_pid = fork();
	if(child_pid < 0){
		printf("Fel vid skapande av process");
		exit(1);
	}
	return child_pid;
}

/************** Registrerar att vi fått en signal *****************/
/*Går att bygga ut om man vill att flera olika fall ska hanteras, som 
i AnvandbaraSysanrop. Just nu tar den bara emot vilken signalkod som
skickas och säger sedan åt programmet att start om main (utan SA_RESTART
kommer den köra om senaste anrop.*/
void regSigHandler(int sigCode){
	int retValue;
	struct sigaction sigParams;
	sigParams.sa_handler = sigHandler;
	sigParams.sa_flags = SA_RESTART;
	retValue = sigaction(sigCode, &sigParams, (void *) 0);
	if (-1 == retValue){
		perror("Något gick fel med sigaction");
		exit(1);
	}
}

/************** Hanterar vad som sker vid CTRL-C **********************/
/*Just nu gör vi ingenting eftersom vi vill att det inte ska hända något
då man trycker CTRL-C om vi bara står innen i vår kommandotolk utan att 
ha några andra processer igång.*/
void sigHandler(int sigCode){

}

/******************** Kolla Bakgrundsprocsser *************************/
/*Skulle det ligga avstängda bakgrundsprocesser hanterar denna funktion 
att plocka bort dem. Skulle det finnas aktiva bakgrundsprocesser går dessa 
att stänga genom att trycka CTRL-C. Vi kollar även om bakgrundsprocesserna
stängdes av sig själva eller om de stängdes av någon signal (tex SIGINT om
vi har tryckt CTRL-C*/
void cleanBgProcess(){
	int status;
	pid_t bgProcess;
    while ((bgProcess = waitpid(-1, &status, WNOHANG)) > 0)
    {
    	if (0 == status){
    		printf("Bakgrundsprocess %d avslutad\n", bgProcess);
    	}
    	else{
    		printf("Bakgrundsprocess %d avlsutad med signal %d", bgProcess, status);
    	}
    }
}
