#!/usr/bin/env python

# This is the world's first (I think) implementation of the
#
# ZOMBIE-ORIENTED MACHINE BEING INTERFACE ENGINE
#
# You know, like Hex, but with evil beings! Written in Python to offset the evil a bit.
#
# Evil necromancers might want to go here:
#  http://www.dangermouse.net/esoteric/zombie.html
# to read the specification, which was made over two years ago, and still nobody
# dared implement it.


import re,sys,thread,time,random

# regexps recognizing syntax elements
comment_re = re.compile("\{.*?\}", re.DOTALL)
declaration_re = re.compile(r'([A-Za-z0-9_\-]*?)\s+is\s+an?\s+(zombie|enslaved undead|' + \
                            r'ghost|restless undead|vampire|free-willed undead' + \
                            r'|demon|djinn)', re.I)

task_re = re.compile("task\s+([A-Za-z0-9_-]*)", re.I)
remember_re = re.compile("remember\s+(.*)", re.I)

string_re = re.compile('".*?"')
integer_re = re.compile('[\-0-9\.]+')

kill=False

# error message
def die(msg):
   global kill
   print "--- Fatal error: %s" % msg
   kill=True
   sys.exit()

   
# split line according to whitespace, but keep strings intact

def splitline(string):
   cmds = []
    
   # break line up into pieces
   prev_whitespace = False
   in_string = False
   temp =  ''
   for c in reversed(range(0,len(string))):
      if string[c]==' ' and not (prev_whitespace or in_string):
         prev_whitespace = True
         cmds.insert(0,temp)
         temp = ""
      elif string[c]!=' ' or in_string:
         prev_whitespace = False
         if string[c]=='"': in_string = not in_string
         temp = string[c]+temp
   cmds.insert(0,temp)
   
   return cmds
   
   
# Entity objects
class Entity:
   environment = None
   name = ''
   memory = None
   tasks = []
   def runtasks(self):
      self.active=True
      # makes a thread; runs the tasks as the specific entity type does.
      thread.start_new_thread( self.taskthread, () )
   
   def taskthread(self):
      pass; # overloaded by individual entities that have their own way of doing things

   def activate(self): 
      self.active=True
      self.runtasks
      
   def banish(self):
      self.active=False # task threads check for this and stop
      
class Undead (Entity):
   pass # which is what we make at undead people, of course :-)
   
class Zombie (Undead):
   active = False # zombies require animating
   
   # zombies run their tasks in order
   def taskthread(self):
      while self.active and (not kill):
         for task in [t for t in self.tasks if t.active]:
            if not self.active: break
            task.run()
            task.active=False
         time.sleep(.05)
         if not [t for t in self.tasks if t.active]: self.active = False

class Ghost (Undead):
   active = False # ghosts require desturbing
   
   # ghosts run their tasks in order, but may wait before starting a new task
   def taskthread(self):
      while self.active and (not kill):
         for task in [t for t in self.tasks if t.active]:
            time.sleep(int(random.random() * 60))
            if not self.active: break
            task.run()
            task.active=False
         if not [t for t in self.tasks if t.active]: self.active = False
            
   
class Vampire (Undead):
   active = True # vampires are active immediately
   
   # vampires process their tasks in random order
   def taskthread(self):
      random.shuffle(self.tasks) # tasks in random order
      while self.active  and (not kill):
         for task in [t for t in self.tasks if t.active]:
            if not self.active: break
            task.run()
            task.active=False
         time.sleep(.05)
         if not [t for t in self.tasks if t.active]: self.active = False

class Demon (Entity):
   active = True # demons are always active

   # demons process their tasks in random order, and sometimes multiple times
   def taskthread(self):
      random.shuffle(self.tasks) # tasks in random order
      
      # may run multiple tasks at once (this may cause weird things to happen with
      # threading, but I don't care; you shouldn't trust demons anyway)
      if bool(int(random.random() * 4)):
         thread.start_new_thread( self.taskthread, () )
         
      while self.active and (not kill):
         for task in [t for t in self.tasks if t.active]:
            if not self.active: break
            task.run()
            task.active= not bool(int(random.random()*4)) # 1 in 4 chance a task will be repeated
         time.sleep(.05)
         if not [t for t in self.tasks if t.active]: self.active = False
   
class Djinn (Entity):
   active = True # djinn are always active
     
   def taskthread(self):
      random.shuffle(self.tasks) # tasks in random order
      
      # may run multiple tasks at once (this may cause weird things to happen with
      # threading, but I don't care; you shouldn't trust demons anyway)
      if bool(int(random.random() * 4)):
         thread.start_new_thread( self.taskthread, () )
         
      while self.active and (not kill):
         for task in [t for t in self.tasks if t.active]:
            if not self.active: break
            # tasks may not run at all
            if task.active: task.active= not bool(int(random.random()*4)) # 1 in 4 chance a task will be repeated
            if task.active: task.run() 
         time.sleep(.05)
         if not [t for t in self.tasks if t.active]: self.active = False
   
# Task objects store tasks and can be run
class Task:
   entity = None
   lines = []
   name = ''
   active = False
   
   def __init__(self):
      self.commands = \
         {'animate': self.c_animate,
          'banish': self.c_banish,
          'disturb': self.c_disturb,
          'forget': self.c_forget,
          'invoke': self.c_invoke,
          'moan': self.c_moan,
          'remember': self.c_remember,
          'say': self.c_say}
          
   # commands
   def c_animate(self,stack):
      if stack and len(stack)>=1 and isinstance(stack[-1],Entity):
         if not isinstance(stack[-1], Zombie):
            die("task %s, entity %s: attempt to animate non-zombie entity %s." % \
                (self.name, self.entity.name, stack[-1].name))
         stack.pop().activate()
      else:
         if not isinstance(self.entity, Zombie):
            die("task %s, entity %s: attempt to animate non-zombie entity %s." % \
                (self.name, self.entity.name, self.entity.name))
         self.entity.activate()
   
   def c_banish(self,stack):
      if stack and len(stack)>=1 and isinstance(stack[-1],Entity):
         stack.pop().banish()
      else:
         self.entity.banish()
         
   def c_disturb(self,stack):
      if stack and len(stack)>=1 and isinstance(stack[-1],Entity):
         if not isinstance(stack[-1], Ghost):
            die("task %s, entity %s: attempt to disturb non-ghost entity %s." % \
                (self.name, self.entity.name, stack[-1].name))
         stack.pop().activate()
      else:
         if not isinstance(self.entity, Ghost):
            die("task %s, entity %s: attempt to disturb non-ghost entity %s." % \
                (self.name, self.entity.name, self.entity.name))
         self.entity.activate()
      
   def c_forget(self,stack):
      if stack and len(stack)>=1 and isinstance(stack[-1],Entity):
         stack.pop().memory=None
      else:
         self.entity.memory=None
   
   def c_invoke(self,stack):
      if stack and len(stack)>=1 and isinstance(stack[-1],Entity):
         stack.pop().activate()
      else:
         self.entity.activate()
   
   def c_moan(self,stack):
      if stack and len(stack)>=1 and isinstance(stack[-1],Entity):
         #sys.stdout.write(str(stack[-1].memory))
         stack.append(stack.pop().memory)
      else:
         stack.append(self.entity.memory)
         #sys.stdout.write(str(self.entity.memory))
   
   def c_remember(self,stack):
      #print "\n Remember :%s: " % str(stack)
      
      if stack and len(stack)>=1 and isinstance(stack[-1],Entity):
         theEntity = stack[-1]
         values = stack[:-1]
      else:
         theEntity = self.entity
         values = stack
      
      total = 0
      for value in values:
         if isinstance(value, int):
            # numbers simply get added
            total += value
         elif isinstance(value, str):
            # strings with numbers in get added, others get ignored
            try: total += int(value)
            except: pass
         elif isinstance(value, Entity):
            # entities get their memories added
            try: total += value.memory
            except: pass
         else:
            # just try to add it, ignore if not possible
            try: total += value
            except: pass
         
      for i in range(0,len(values)): stack.pop()
      
      theEntity.memory = total
   
   def c_say(self, stack):
      if not stack or (isinstance(stack[0],Entity) and len(stack)==1):
         die("task %s, entity %s: argument error for SAY: nothing to say." % \
             (self.name,self.entity.name))
      if isinstance(stack[0],Entity):
         sys.stdout.write(str(stack[1]))
         if not isinstance(stack[1], str): sys.stdout.write(" ")
      else:
         sys.stdout.write(str(stack[0]))
         if not isinstance(stack[0], str) or isinstance(stack[0], float): sys.stdout.write(" ")
      
   # do task
   def run(self):
      try:self._run()
      except:die("task %s, entity %s: something weird happened; program terminated to insure safety." \
                 % (self.name, self.entity.name))
   def _run(self):
      line_no = 0;
      while (line_no < len(self.lines)):
         cmds = splitline(self.lines[line_no])
         stack = []
         
         #print self.lines[line_no]
         
         for cmd in reversed(cmds):
            if cmd.lower() in ['animate', 'banish', 'disturb', 'forget',
                               'invoke', 'moan', 'remember', 'say']: 
               self.commands[cmd.lower()](stack)
            elif cmd.lower() == "remembering":
               if len(stack)>=1:
                  if isinstance(stack[-1],Entity):
                     if len(stack)>=2:
                        val = int(stack[-1].memory == stack[-2])
                        stack.pop()
                        stack.pop()
                        stack.append(val)
                     else:
                        die("task %s, entity %s: argument error for REMEMBERING." % \
                            (self.name, self.entity.name))
                  else:
                     stack.append( int(stack.pop() == self.entity.memory) )
               else:
                  die("task %s, entity %s: argument error for REMEMBERING." % \
                      (self.name, self.entity.name))
                      
            elif cmd.lower() == "stumble":
               return;
            elif cmd.lower() == "rend":
               try:
                 a=stack.pop()
                 stack.append(stack.pop() / a)
               except:
                 die("task %s, entity %s: argument error for REND." % (self.name,self.entity.name))
            elif cmd.lower() == "turn":
               try:
                 stack.append(-stack.pop())
               except:
                 die("task %s, entity %s: argument error for TURN." % (self.name,self.entity.name))
            elif cmd.lower() == "around":
               t = 1
               try:
                  while t:
                     line_no -= 1
                     if self.lines[line_no].split()[0].lower() in ['around', 'until']: t += 1
                     if self.lines[line_no].split()[0].lower() == 'shamble': t -= 1
                  #break
               except:
                  die("task %s, entity %s: unbalanced loop." % (self.name, self.entity.name))
            elif cmd.lower() == "until":
               if (not stack) or (not stack[0]):
                  t = 1
                  try:
                     while t:
                        line_no -= 1
                        if self.lines[line_no].split()[0].lower() == 'shamble': t -= 1
                        if self.lines[line_no].split()[0].lower() in ['around', 'until']: t += 1
                     #break
                  except:
                     die("task %s, entity %s: unbalanced loop." % (self.name, self.entity.name))
            elif cmd.lower() == "taste":
               if (not stack) or (not stack[0]):
                  t = 1
                  try:
                     while t:
                        line_no += 1
                        if self.lines[line_no].split()[0].lower() == 'taste': t += 1
                        if self.lines[line_no].split()[0].lower() == 'spit': t -= 1
                        if self.lines[line_no].split()[0].lower() == 'bad' and t == 1:
                           break
                  except:
                     die("task %s, entity %s: unbalanced taste/spit." %(self.name,self.entity.name))
            elif cmd.lower() == "bad":
               # this'll only happen if "taste" didn't send it to "bad"; so skip to "spit"
               t = 1
               try:
                  while t:
                     line_no += 1
                     if self.lines[line_no].split()[0].lower() == 'taste': t += 1
                     if self.lines[line_no].split()[0].lower() == 'spit': t -= 1
               except:
                  die("task %s, entity %s: unbalanced taste/spit." % (self.name, self.entity.name))
            elif cmd.lower() in ['shamble', 'good', 'spit']:
               # syntax elements that do nothing themselves when reached
               pass;
            else:
               # it's a value
               if string_re.match(cmd): 
                  stack.append(cmd[1:-1].replace('\\n','\n').replace('\\t','\t'))
               elif integer_re.match(cmd): 
                  stack.append(float(cmd))
                  if stack[-1] == int(stack[-1]): stack[-1] = int(stack[-1])
               elif str(cmd) in self.entity.environment.entities:
                  stack.append(self.entity.environment.entities[cmd])
               else:
                  die("task %s, entity %s: '%s' does not exist." % (self.name,self.entity.name,cmd))
           
         line_no += 1
         
         
         
               

# The environment in which the entities do their tasks. 
# Not necessarily a graveyard, one can work around that
# by not using any undead.
class Environment:
   entities = {}
  
   def __init__(self, file):
      # read file
      try:
         f = open(file, "r")
         code = f.read()
         f.close()
      except:
         die("cannot open file %s." % file)
      
      # parse code
      self.parse(code)
      
   def run(self):
      global kill
      
      # activate all entities that should be activated
      [self.entities[e].runtasks() for e in self.entities if self.entities[e].active]
      
      # keep the main thread running until all entities are done
      while (not kill) and [e for e in self.entities if self.entities[e].active]:
         time.sleep(1)
         if kill: sys.exit()
         
      print "\n"
      
      
   # Make entities according to the code supplied
   def parse(self, code):
      currentEntity = None
      inEntity = False
      
      currentTask = None
      
      # remove comments from code
      code = comment_re.sub(lambda x:'', code)
      
      # split into lines and remove whitespace
      lines = [a.strip() for a in code.split("\n") if not a.strip() == '']
      
      line_no = 0
      while line_no < len(lines):
  #       print "LINE : '%s'" % lines[line_no]
         
         if not inEntity:
            a = declaration_re.match(lines[line_no])
            if a:
               if lines[line_no+1].lower() == 'summon':
                  # we're in an entity declaration block.
                  # start a new entity, if possible
               
                  if a.group(1) in self.entities:
                     die("line %d: entity '%s' is already defined." % (line_no, a.group(1)))
               
                  if a.group(2).lower() in ['zombie', 'enslaved undead']: type=Zombie
                  elif a.group(2).lower() in ['ghost', 'restless undead']: type=Ghost
                  elif a.group(2).lower() in ['vampire', 'free-willed undead']: type=Vampire
                  elif a.group(2).lower() == 'demon': type=Demon
                  elif a.group(2).lower() == 'djinn': type=Djinn
                  else:
                     die("line %d: '%s' is not a valid entity type." % a.group(2))
                  currentEntityName = a.group(1)
                  currentEntity = type()
                  currentEntity.name = currentEntityName
                  line_no += 1
                  inEntity = True
               else:
                  die("line %d: entity declaration incomplete, missing SUMMON." % line_no)
            else:
               die("line %d: only entity declarations may appear outside of entities." % line_no)
         else:
            a = task_re.match(lines[line_no])
            b = remember_re.match(lines[line_no])
            
            if a:
               #read the task, store it in the entity
               currentTask = Task()
               currentTask.name = a.group(1)
                                    
               while True:
                  line_no += 1
                  if lines[line_no].lower() in ['bind', 'animate']:
                     currentTask.active = lines[line_no].lower() == 'animate'
                     currentTask.entity = currentEntity
                     currentEntity.tasks += [currentTask]
                     break
                  else:
                     currentTask.lines += [lines[line_no]]
                   
            elif b:
               #default value
               if b.group(1)[0] == '"' and b.group(1)[-1] == '"':
                  # string
                  currentEntity.memory = b.group(1)[1:-1]
               else:
                  try: currentEntity.memory = int(b.group(1))
                  except ValueError:
                     die("line %d: REMEMBER outside of task may only use a constant." % line_no)
            
            else:
               if not lines[line_no].lower() in ['animate', 'bind', 'disturb']:
                  die("line %d: not allowed outside of task." % line_no)
               else:
                  inEntity = False
                  
               if lines[line_no].lower() != 'bind':
                  # a state change (to active) is required
                  if not isinstance(currentEntity, Undead):
                     die(("line %d: safety check failed: free-willed non-undead *must* be bound " +\
                         "or all hell will break loose quite literally.") % line_no)
                  
                  if lines[line_no].lower() == 'animate' and not isinstance(currentEntity,Zombie) \
                  or lines[line_no].lower() == 'disturb' and not isinstance(currentEntity,Ghost) :
                     die(("line %d: type error: you are either animating a ghost or disturbing " +\
                         "a zombie; both aren't a very good idea.") % line_no)
                       
                  # The fact that we're still here means that the program hasn't been killed.
                  # That means the safety checks have passed, we can safely activate the entity.
                  currentEntity.active = True
               
               currentEntity.environment = self
               self.entities[currentEntity.name] = currentEntity
         
         line_no += 1
         
         
             
if __name__ == '__main__':
   if (len(sys.argv)) != 2: die("argument error.")
   env = Environment(sys.argv[1])
   env.run()
