import Link from "next/link";

const IfElseStatements = () => {
  return (
    <div className="p-8">
      {/* Musoro (Header) */}
      <h1 className="text-4xl font-bold mb-4">🔀 Kana - Kana Kuti (If-Else) muShona Programming Language</h1>
      <p className="text-lg text-gray-700">
        **Kana** inobatsira kuti chirongwa chisarudze kuita chimwe chinhu zvinoenderana nemamiriro ezvinhu.
      </p>

      {/* Chirevo (What is If-Else?) */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">ℹ️ Chii chinonzi Kana - Kana Kuti?</h2>
        <p className="text-gray-700">
          Kana **mamiriro ezvinhu** akasangana nechakatarwa, tinogona kuita chimwe chinhu.
          Kana zvikasadaro, tinogona kuita chimwe chakasiyana.
        </p>
      </section>

      {/* Basic Syntax */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">📝 Syntax ye Kana - Kana Kuti</h2>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Muenzaniso we "kana"
kana (mamiriro) {
  # Ita izvi kana mamiriro acho achokwadi (true)
} kana_kuti {
  # Ita izvi kana mamiriro acho anyepa (false)
}`}
          </code>
        </div>
      </section>

      {/* Simple Example */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">✅ Muenzaniso Wakapfava</h2>
        <p className="text-gray-700">Muenzaniso unoratidza kushanda kwe **kana - kana_kuti**.</p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Tichatarisa kana nhamba iri kukunda kana kuti kwete
zita nhamba = 10

kana (nhamba > 5) {
  nyora("Nhamba yakakura kupfuura 5")
} kana_kuti {
  nyora("Nhamba idiki kana yakaenzana ne 5")
}`}
          </code>
        </div>
      </section>

      {/* Nested If-Else Example */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">🔄 Kana Mukati me Kana (Nested If-Else)</h2>
        <p className="text-gray-700">Kana tichida kutarisa mamiriro akawanda.</p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kuenzanisa kukura kwe nhamba
zita nhamba = 15

kana (nhamba > 10) {
  nyora("Nhamba yakakura kupfuura 10")
} kana (nhamba > 5) {
  nyora("Nhamba iri pakati pe 6 ne 10")
} kana_kuti {
  nyora("Nhamba idiki kana yakaenzana ne 5")
}`}
          </code>
        </div>
      </section>

      {/* Boolean Conditions */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">🔢 Mamiriro e Boolean (True/False)</h2>
        <p className="text-gray-700">
          Tinogona kushandisa **boolean values** (chokwadi kana manyepo).
        </p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kuenzanisa kuti munhu akura here
zita akura = true

kana (akura) {
  nyora("Unobvumirwa kupinda")
} kana_kuti {
  nyora("Haubvumidzwe kupinda")
}`}
          </code>
        </div>
      </section>

      {/* Logical Operators */}
      <section className="mt-6">
        <h2 className="text-2xl font-semibold mb-3">🔀 Kushandisa Logical Operators</h2>
        <p className="text-gray-700">Tinogona kushandisa **&& (uye)** uye **|| (kana)**.</p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Kuenzanisa makore evanhu
zita makore = 20

kana (makore > 18 && makore < 65) {
  nyora("Unogona kushanda")
} kana_kuti {
  nyora("Haubvumidzwe kushanda")
}`}
          </code>
        </div>
      </section>

      {/* Navigation Links */}
      <section className="mt-8">
        <Link href="/loops">
          <span className="bg-blue-600 text-white px-6 py-2 rounded-lg text-lg font-semibold cursor-pointer hover:bg-blue-700">
            👉 Dzidza nezve Ita (Loops)
          </span>
        </Link>
      </section>
    </div>
  );
};

export default IfElseStatements;
