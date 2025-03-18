import Link from "next/link";

const Variables = () => {
  return (
    <div className="p-8">
      {/* Musoro (Header) */}
      <h1 className="text-4xl font-bold mb-4">📌 Zita (Variable) muShona Programming Language</h1>
      <p className="text-lg text-gray-700">
        Zita (variable) rinobatsira kuchengeta **data** yatingashandisa muchirongwa.
      </p>

      {/* Sei Tichishandisa Zita? */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">🔍 Sei Tichishandisa Zita?</h2>
        <ul className="list-disc list-inside text-gray-700">
          <li>Kuchengetedza manhamba, mashoko, kana **data** yechirongwa.</li>
          <li>Kuita kuti kodhi yedu inyore kunzwisisa uye kushanda nayo.</li>
          <li>Kuita kuti chirongwa chishande nemaitiro akasiyana pasina kushandura kodhi yese.</li>
        </ul>
      </section>

      {/* Mutemo Wekugadzira Zita */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">⚖️ Mutemo Wekugadzira Zita</h2>
        <p className="text-gray-700">
          - Zita rinofanira kutanga nerugwaro (**A-Z, a-z**).  
          - Rinogona kuva nemanamba (**0-9**) uye underscore (**_**), asi risingatangi nenhamba.  
          - **Hapana spaces kana mavara asiri emutauro** (e.g., `@, $, %` hazvitenderwe).  
        </p>
      </section>

      {/* Muenzaniso (Example Code) */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">💻 Muenzaniso Wekushandisa Zita</h2>
        <p className="text-gray-700 mb-4">
          Heino kodhi inoratidza mashandisiro ezita mukodhi:
        </p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kugadzira uye kushandisa zita
zita a = 10
zita mazwi = "Mhoro nyika!"
nyora(a)
nyora(mazwi)`}
          </code>
        </div>
      </section>

      {/* Explanations */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">📌 Tsanangudzo</h2>
        <p className="text-gray-700">
          - **zita a = 10** → Rinogadzira zita **a** rine kukosha **10**  
          - **zita mazwi = "Mhoro nyika!"** → Rinogadzira zita **mazwi** rine mashoko  
          - **nyora(a)** → Rinonyora kukosha kwe **a**  
          - **nyora(mazwi)** → Rinonyora kukosha kwe **mazwi**  
        </p>
      </section>

      {/* Link to next section */}
      <section className="mt-8">
        <Link href="/functions">
          <span className="bg-blue-600 text-white px-6 py-2 rounded-lg text-lg font-semibold cursor-pointer hover:bg-blue-700">
            👉 Dzidza nezve Mabasa (Functions)
          </span>
        </Link>
      </section>
    </div>
  );
};

export default Variables;
