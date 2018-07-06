/*
- Singleton pattern. Atat PhysX cat si Engine nu pot avea decat o singura instanta in program
---Motivele pot fi multiple:
----Engine: nu putem crea decat o singura instanta de randare / o singura fereastra OpenGL (excluzand entitatea GLUI)
            / un singur "modul" autorizat pentru modificarea parametrilor de vizualizare
----PhysX: programul meu necesita o singura instanta de simulator fizic accesibil din orice punct al aplicatiei
           datorita posibilitatii de schimbare dinamica a numarului de actori de catre Engine, Input Manager, Model Importer etc.

- Decorator pattern. Adaugam responsabilitati modelului OBJ importat astfel:
--- Orice model importat din format OBJ trebuie "gatit" de catre PhysX.
--- Aceasta pregatire a meshei poate fi influentata de diversi parametri:
----- BodyMass | ActorPosition | TriangleDescriptor | ConvexDescriptor etc. 
------Nici unul din acesti parametri nu este necesar dar recomandat pentru crearea unui "actor fizic"

Folosim PhysXDecorator pentru definirea claselor nucleu/decorator:
<<class PendingActor>>
-este clasa noastra abstracta din care derivam actorul si decoratorii
<<class PendingConcreteActor>> 
-este implementarea concreta a clasei PendingActor si reprezinta nucleul actorului final
-functia creatActor returneaza actorul gata decorat
<<class DecorateActor>>
-este clasa prototip pentru decoratori derivata tot din clasa PendingActor
-Fiecare functie din aceasta clasa apeleaza functie echivalenta din PendingConcretActor pe care il ia ca parametru

<<class ****Decorator>>
-reprezinta lista de decoratori posibili
-putem adauga cati vrem sau nici unul actorului final pe care il vom simula fizic
--daca folosim vreun decorator, trebuie neaparat sa ii configuram si parametrii actorului final asociati decoratorului
---Ex: PositionDecorator atrage dupa sine folosirea actor->setPos(NxVec3(x,y,z));


   

