import Link from "next/link";

const SyntaxGuide = () => {
  return (
    <div className="p-8">
      {/* Musoro (Header) */}
      <h1 className="text-4xl font-bold mb-4">📖 Gwaro re Syntax</h1>
      <p className="text-lg text-gray-700">
        Mutauro weShona Programming Language unonyanya kushandisa **mazwi echiShona** 
        kuti munhu wese anzwisise zviri nyore.
      </p>

      {/* Zvinhu Zvikuru mu Syntax */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">🔤 Zvinhu Zvikuru mu Syntax</h2>
        <ul className="list-disc list-inside text-gray-700">
          <li>Kushandisa **mazwi echiShona** pamirairo nemashandisirwo.</li>
          <li>Kuchengetedza **data** mukati mezita (**variables**).</li>
          <li>Kushandisa **mabasa (functions)** kugadzirisa data.</li>
          <li>Kunyora zvinobuda (output) uchishandisa **nyora()**.</li>
        </ul>
      </section>

      {/* Kugadzira Zita (Variables) */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">📝 Kugadzira Zita (Variables)</h2>
        <p className="text-gray-700">MuShona Programming Language, tinogadzira zita (variable) ne **zita** keyword:</p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kugadzira zita 
zita a = 10
zita mashoko = "Mhoro nyika!"
nyora(a)  # Inobudisa 10
nyora(mashoko)  # Inobudisa "Mhoro nyika!"
`}
          </code>
        </div>
      </section>

      {/* Mashandisirwo eNyora Function */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">📢 Kunyora Output ne **nyora()**</h2>
        <p className="text-gray-700">
          **nyora()** inoshandiswa kubudisa (output) zvataisa mukodhi.
        </p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kunyora ku screen
nyora("Shona Programming Language iripo!")`}
          </code>
        </div>
      </section>

      {/* Control Flow */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">🔄 Control Flow (Kana, Pakati, Ita)</h2>
        <p className="text-gray-700">Tinoshandisa **kana, pakati, ita** kuzadzisa mirairo.</p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kana (if) muShona Programming Language
zita nhamba = 10

kana (nhamba > 5) {
  nyora("Nhamba yakakura kupfuura 5")
} kana_kuti {
  nyora("Nhamba idiki")
}`}
          </code>
        </div>
      </section>

      {/* Mabasa (Functions) */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">🛠️ Kugadzira Mabasa (Functions)</h2>
        <p className="text-gray-700">
          Mabasa anobatsira kuti kodhi yedu ive yakachena uye nyore kushandisa.
        </p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kugadzira basa
basa kuwedzera(a, b) {
  dzosa a + b
}

nyora(kuwedzera(5, 3))  # Inobudisa 8`}
          </code>
        </div>
      </section>

      {/* Navigation kuNext Section */}
      <section className="mt-8">
        <Link href="/variables">
          <span className="bg-blue-600 text-white px-6 py-2 rounded-lg text-lg font-semibold cursor-pointer hover:bg-blue-700">
            👉 Dzidza Zvimwe Nezve Zita (Variables)
          </span>
        </Link>
      </section>
    </div>
  );
};

export default SyntaxGuide;
