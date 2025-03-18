import Link from "next/link";

const Home = () => {
  return (
    <div className="p-8">
      {/* Chikamu chekutanga (Hero Section) */}
      <section className="bg-gray-100 p-10 rounded-lg shadow-md text-center">
        <h1 className="text-4xl font-bold mb-4">
          Mauya ku <span className="text-blue-600">Shona Programming Language</span>
        </h1>
        <p className="text-lg text-gray-700 mb-6">
          Kuunza mukana kune wese munhu kuti adzidze programming mumutauro wemuno.
        </p>
        <Link href="/installation">
          <span className="bg-blue-600 text-white px-6 py-2 rounded-lg text-lg font-semibold cursor-pointer hover:bg-blue-700">
            Tanga Pano
          </span>
        </Link>
      </section>

      {/* Zvimiro Zvikuru (Key Features) */}
      <section className="mt-10">
        <h2 className="text-2xl font-semibold mb-4">🚀 Sei Uchisarudza Mutauro Uyu?</h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          <div className="p-6 bg-white shadow-md rounded-lg">
            <h3 className="text-xl font-bold">🌍 Kuwedzera Mikana</h3>
            <p className="text-gray-600">Dzidza coding uchishandisa mutauro waunonzwa.</p>
          </div>
          <div className="p-6 bg-white shadow-md rounded-lg">
            <h3 className="text-xl font-bold">📚 Kutsigira Dzidzo Yevana</h3>
            <p className="text-gray-600">Vana vanogona kudzidza coding vachiri kuchikoro pasina chipingamupinyi chemutauro.</p>
          </div>
          <div className="p-6 bg-white shadow-md rounded-lg">
            <h3 className="text-xl font-bold">💡 Kukurudzira Kugadzira</h3>
            <p className="text-gray-600">Kuwedzera hunyanzvi hwevanhu nekuva nechokwadi chekuti vanonzwisisa zvakadzama.</p>
          </div>
        </div>
      </section>

      {/* Muenzaniso weKodhi (Example Code) */}
      <section className="mt-10">
        <h2 className="text-2xl font-semibold mb-4">💻 Muenzaniso weKodhi</h2>
        <p className="text-gray-700 mb-4">Heino kodhi yekutanga muShona:</p>
        <div className="bg-gray-900 text-white p-4 rounded-lg font-mono">
          <code>
            {`# Fungidziro yeveruzhinji 
tanga nyora("Mhoro Nyika!")`}
          </code>
        </div>
      </section>
    </div>
  );
};

export default Home;
