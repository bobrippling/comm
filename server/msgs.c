void handle_message(int idx, char *data)
{
	int i;

	if(!*data){
		TO_CLIENT(idx, "ERR need message");
		return;
	}

	for(i = 0; i < nclients; i++)
		if(clients[i].state == ACCEPTED)
			/* send back to idx */
			toclientf(i, "%s", data);

	if(nclients == 1)
		TO_CLIENT(idx, "MESSAGE " LONELY_MSG);
}

void handle_privmsg(int idx, char *name)
{
	int i;
	char *sep = strchr(name, GROUP_SEPARATOR);

	if(!sep)
		TO_CLIENT(idx, "ERR no separator for PRIVMSG");
	else{
		*sep = '\0';

		if(!strcmp(clients[idx].name, name))
			TO_CLIENT(idx, "ERR can't privmsg yourself");
		else{
			for(i = 0; i < nclients; i++)
				if(clients[i].state == ACCEPTED && idx != i &&
						!strcmp(clients[i].name, name)){
					toclientf(i, "PRIVMSG %s", sep + 1);
					break;
				}

			if(i == nclients)
				TO_CLIENT(idx, "ERR no such client");
			else
				/* back to origin */
				toclientf(idx, "PRIVMSG %s", sep + 1);
		}
	}
}

void handle_client_list(int idx, char *data)
{
	int i;

	if(*data){
		TO_CLIENT(idx, "ERR invalid message");
		return;
	}

	TO_CLIENT(idx, "CLIENT_LIST_START");
	for(i = 0; i < nclients; i++)
		if(i != idx && clients[i].state == ACCEPTED){
			if(clients[i].colour)
				toclientf(idx, "CLIENT_LIST %s%c%s",
						clients[i].name, GROUP_SEPARATOR, clients[i].colour);
			else
				toclientf(idx, "CLIENT_LIST %s", clients[i].name);
		}
	TO_CLIENT(idx, "CLIENT_LIST_END");
}

void handle_rename(int idx, char *name)
{
	if(!*name || strlen(name) > MAX_NAME_LEN)
		TO_CLIENT(idx, "ERR need name/name too long");
	else if(!validname(name))
		TO_CLIENT(idx, "ERR invalid name");
	else{
		if(!strcmp(clients[idx].name, name)){
			TO_CLIENT(idx, "ERR can't rename to the same name");
		}else{
			int i, good = 1;

			for(i = 0; i < nclients; i++)
				if(clients[i].state == ACCEPTED && !strcmp(name, clients[i].name)){
					good = 0;
					break;
				}

			if(good){
				char oldname[MAX_NAME_LEN];
				char *new;
				strncpy(oldname, clients[idx].name, MAX_NAME_LEN);

				new = realloc(clients[idx].name, strlen(name) + 1);
				if(!new)
					longjmp(allocerr, 1);
				strcpy(clients[idx].name = new, name);
				for(i = 0; i < nclients; i++)
					if(clients[i].state == ACCEPTED)
						/* send back to idx too, to confirm */
						toclientf(i, "RENAME %s%c%s", oldname, GROUP_SEPARATOR, new);
			}else
				TO_CLIENT(idx, "ERR name already taken");
		}
	}
}

void handle_colour(int idx, char *col)
{
	if(!*col)
		TO_CLIENT(idx, "ERR need colour");
	else{
		char *new = realloc(clients[idx].colour, strlen(col)+1);
		int i;

		if(!new)
			longjmp(allocerr, 1);

		strcpy(clients[idx].colour = new, col);
		for(i = 0; i < nclients; i++)
			if(i != idx && clients[i].state == ACCEPTED)
				toclientf(i, "COLOUR %s%c%s",
						clients[idx].name, GROUP_SEPARATOR, clients[idx].colour);

	}

}

void handle_kick(int idx, char *name)
{
	if(clients[idx].isroot){
		int i, found = 0;

		if(!strlen(name))
			TO_CLIENT(idx, "ERR need name to kick");
		else{
			for(i = 0; i < nclients; i++)
				if(clients[i].state == ACCEPTED &&
						!strcmp(clients[i].name, name)){
					found = 1;
					break;
				}

			if(!found)
				toclientf(idx, "ERR name \"%s\" not found", name);
			else{
				int j;
				for(j = 0; j < nclients; j++)
					if(j != i)
						toclientf(j, "INFO %s was kicked", name);
				TO_CLIENT(i, "INFO you have been kicked");
				DEBUG(idx, DEBUG_STATUS, "<- (%s) kicked %s",
							clients[idx].name, name);
				svr_hup(i);
			}
		}
	}else
		TO_CLIENT(idx, "ERR not root");

}

void handle_su(int idx, char *reqpass)
{
	if(clients[idx].isroot){
		TO_CLIENT(idx, "INFO dropping root");
		DEBUG(idx, DEBUG_STATUS, "%s dropping root", clients[idx].name);
		clients[idx].isroot = 0;
	}else{
		if(!*reqpass)
			TO_CLIENT(idx, "ERR Need password");
		else if(strcmp(reqpass, glob_pass)){
			TO_CLIENT(idx, "ERR incorrect root passphrase");
			DEBUG(idx, DEBUG_STATUS, "%s incorrect root pass %s", clients[idx].name, reqpass);
		}else{
			clients[idx].isroot = 1;
			TO_CLIENT(idx, "INFO root login successful");
			DEBUG(idx, DEBUG_STATUS, "root login: %s", clients[idx].name);
		}
	}
}

void handle_typing(int idx, char *data)
{
	int i;

	if(!strcmp(data, "1"))
		clients[idx].typing = 1;
	else if(!strcmp(data, "0"))
		clients[idx].typing = 0;
	else{
		TO_CLIENT(idx, "ERR invalid typing message");
		return;
	}

	for(i = 0; i < nclients; i++)
		if(i != idx)
			toclientf(i, "TYPING %d%s", clients[idx].typing, clients[idx].name);
}
