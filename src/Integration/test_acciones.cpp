#include <boost/test/unit_test.hpp>
#include <iostream>

// Creamos un bloque (Suite) para agrupar estos tests
BOOST_AUTO_TEST_SUITE(Escenario_Acciones)

BOOST_AUTO_TEST_CASE(Cartera_Conservadora) {
    std::cout << "--> Ejecutando test de Cartera Conservadora...\n";
    // Aquí tu código de simulación...
    BOOST_CHECK(2 + 2 == 4); // Simula que el test pasa
}

BOOST_AUTO_TEST_CASE(Cartera_Agresiva) {
    std::cout << "--> Ejecutando test de Cartera Agresiva...\n";
    // Aquí tu código de simulación...
    BOOST_CHECK(5 > 0); // Simula que el test pasa
}

BOOST_AUTO_TEST_SUITE_END()