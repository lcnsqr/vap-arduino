struct Task {
  char enabled;
  void (*action)(struct Task *);
  unsigned long id;
  unsigned long wait;
  unsigned long last;
  void *data;
  struct Task *next;
};

struct Task *taskAdd(struct Task **tasks, void (*action)(struct Task *), unsigned long wait, void *data){
  struct Task *taskNew = (struct Task*)malloc(sizeof(struct Task));
  taskNew->action = action;
  taskNew->wait = wait;
  taskNew->last = 0;
  taskNew->data = data;
  taskNew->next = NULL;
  taskNew->enabled = 1;
  if ( *tasks == NULL ){
    *tasks = taskNew;
    taskNew->id = 0;
  }
  else {
    struct Task *task = *tasks;
    while ( task->next != NULL ){
      task = task->next;
    }
    task->next = taskNew;
    task->next->id = task->id + 1;
  }
  return taskNew;
}

void taskDel(struct Task **tasks, unsigned long id){
  struct Task *task = *tasks;
  struct Task *taskPrev = NULL;
  while ( task != NULL ){
    if ( task->id == id ){
      if ( task->data != NULL ) free(task->data);
      if ( taskPrev == NULL && task->next == NULL ){
        *tasks = NULL;
        free(task);
        break;
      }
      if ( taskPrev == NULL && task->next != NULL ){
        *tasks = task->next;
        free(task);
        break;
      }
      if ( taskPrev != NULL && task->next == NULL ){
        taskPrev->next = NULL;
        free(task);
        break;
      }
      if ( taskPrev != NULL && task->next != NULL ){
        taskPrev->next = task->next;
        free(task);
        break;
      }
    }
    taskPrev = task;
    task = task->next;
  }
}

struct Task *taskById(struct Task **tasks, unsigned long id){
  struct Task *task = *tasks;
  while ( task != NULL ){
    if ( task->id == id ){
      return task;
    }
    task = task->next;
  }
  return NULL;
}

void taskCheck(struct Task *tasks, unsigned long ticks){
  struct Task *task = tasks;
  while ( task != NULL ){
    if ( task->enabled != 1 ){
      task = task->next;
      continue;
    }
    if ( ticks - task->last >= task->wait ){
      (*(task->action))(task);
      task->last = ticks;
    }
    task = task->next;
  }
}
