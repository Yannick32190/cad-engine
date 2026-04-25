#include <iostream>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Shape.hxx>

int main() {
    std::cout << "Test minimal BRepPrimAPI_MakeBox" << std::endl;
    
    try {
        std::cout << "Création BRepPrimAPI_MakeBox(100, 50, 30)..." << std::endl;
        BRepPrimAPI_MakeBox boxMaker(100.0, 50.0, 30.0);
        
        std::cout << "IsDone() = " << boxMaker.IsDone() << std::endl;
        
        if (boxMaker.IsDone()) {
            TopoDS_Shape shape = boxMaker.Shape();
            std::cout << "✓ Shape créé, IsNull() = " << shape.IsNull() << std::endl;
            return 0;
        } else {
            std::cout << "✗ IsDone() retourne false!" << std::endl;
            return 1;
        }
    } catch (const Standard_Failure& e) {
        std::cout << "✗ Exception OCCT: " << e.GetMessageString() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cout << "✗ Exception std: " << e.what() << std::endl;
        return 1;
    }
}
