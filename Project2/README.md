# Project 02: Message Queue

This is [Project 02] of [CSE.30341.FA21].

## Students

1. Christine Van Kirk (cvankir2@nd.edu)
2. Brett Wiseman (bwisema3@nd.edu)

## Brainstorming

The following are questions that should help you in thinking about how to
approach implementing [Project 02].  For this project, responses to these
brainstorming questions **are not required**.

### Request

1. What data must be allocated and deallocated for each `Request` structure?

2. What does a valid **HTTP** request look like?

### Queue

1. What data must be allocated and deallocated for each `Queue` structure?

2. How will you implement **mutual exclusion**?

3. How will you implement **signaling**?
    
4. What are the **critical sections**?

### Client

1. What data must be allocated and deallocated for each `MessageQueue`
   structure?

2. What should happen when the user **publishes** a message?

3. What should happen when the user **retrieves** a message?

4. What should happen when the user **subscribes** to a topic?

5. What should happen when the user **unsubscribes** to a topic?
    
6. What needs to happen when the user **starts** the `MessageQueue`?

7. What needs to happen when the user **stops** the `MessageQueue`?

8. How many internal **threads** are required?

9. What is the purpose of each internal **thread**?

10. What `MessageQueue` attribute needs to be **protected** from **concurrent**
    access?
    
## Demonstration

> Link to **video demonstration** of **chat application**.
> https://notredame.hosted.panopto.com/Panopto/Pages/Viewer.aspx?id=743c5292-4295-41e2-ba3f-adbb01419d31

## Errata

1. If the user types a message that is longer than the screen, some buggy displaying will happen, but it will not break the message queue.

2. If a user hits enter without typing anything, it will segfault for some reason. We think this is happening with something in ncurses that we do not understand.

3. We do not want the user to add a \n in the message, since it will mess up the line count that moves the messages down the screen, so we do not want to read it in

4. If the person sends a message with that a persons name (name of queue), it will not print on the correct side of the screen, since that is how we check to see if we should print to left or right of screen (left if users own message and right for other users message)

5. Actions are: sub to subscribe, pub to publish, unsub to unsubscribe, /exit or /quit to leave.

[Project 02]:       https://www3.nd.edu/~pbui/teaching/cse.30341.fa21/project02.html
[CSE.30341.FA21]:   https://www3.nd.edu/~pbui/teaching/cse.30341.fa21/
